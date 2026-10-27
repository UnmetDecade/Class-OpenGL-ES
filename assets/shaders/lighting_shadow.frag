#version 330 core

in vec3 v_FragPosWorld;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material uMaterial;
uniform DirLight uDirLight;
uniform PointLight uPointLight;
uniform sampler2D uTexture;
uniform sampler2D u_ShadowMap;
uniform vec3 uViewPos;
uniform mat4 u_LightSpaceMatrix;
uniform float u_BiasScale;
uniform int u_UsePCF;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant
                               + light.linear * distance
                               + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return (ambient + diffuse + specular) * attenuation * albedo;
}

// Returns 1.0 when the fragment is in shadow, 0.0 otherwise.
float ShadowCalculation(vec3 normal, vec3 lightDir)
{
    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPosWorld, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside the light frustum: the border texels (white) would report depth
    // 1.0, so treat the fragment as lit explicitly.
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)) * u_BiasScale, 0.005);

    float shadow = 0.0;

    if (u_UsePCF != 0)
    {
        vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float closestDepth =
                    texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
            }
        }

        shadow /= 9.0;
    }
    else
    {
        float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
        shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
    }

    return shadow;
}

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(uViewPos - v_FragPosWorld);
    vec3 albedo = texture(uTexture, v_TexCoord).rgb;
    vec3 lightDir = normalize(-uDirLight.direction);

    float shadow = ShadowCalculation(normal, lightDir);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);

    vec3 ambient = uDirLight.ambient * uMaterial.ambient * albedo;
    vec3 diffuse = uDirLight.diffuse * diff * uMaterial.diffuse * albedo;
    vec3 specular = uDirLight.specular * spec * uMaterial.specular * albedo;

    // Shadowed fragments keep only the directional ambient term.
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    result += CalcPointLight(uPointLight, normal, v_FragPosWorld, viewDir, albedo);

    FragColor = vec4(result, 1.0);
}
