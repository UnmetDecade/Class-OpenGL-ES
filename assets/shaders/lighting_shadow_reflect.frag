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
uniform samplerCube u_EnvironmentMap;
uniform vec3 uViewPos;
uniform mat4 u_LightSpaceMatrix;
uniform float u_BiasScale;
uniform int u_UsePCF;
uniform float u_Reflectivity;
uniform int u_UseRefraction;

// High-intensity point lights used by the HDR example. The color components
// may exceed 1.0; the count defaults to 0 so other scenes are unaffected.
#define MAX_EXTRA_LIGHTS 3
uniform int u_ExtraLightCount;
uniform vec3 u_ExtraLightPositions[MAX_EXTRA_LIGHTS];
uniform vec3 u_ExtraLightColors[MAX_EXTRA_LIGHTS];

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

vec3 EnvironmentColor(vec3 normal, vec3 viewDir)
{
    vec3 direction = reflect(-viewDir, normal);

    if (u_UseRefraction != 0)
    {
        vec3 refracted = refract(-viewDir, normal, 1.0 / 1.52);
        // Total internal reflection returns a zero vector; keep the reflection.
        if (length(refracted) > 0.0)
        {
            direction = refracted;
        }
    }

    return texture(u_EnvironmentMap, direction).rgb;
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

    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    result += CalcPointLight(uPointLight, normal, v_FragPosWorld, viewDir, albedo);

    for (int i = 0; i < u_ExtraLightCount; ++i)
    {
        PointLight extraLight;
        extraLight.position = u_ExtraLightPositions[i];
        extraLight.constant = 1.0;
        extraLight.linear = 0.35;
        extraLight.quadratic = 0.44;
        extraLight.ambient = vec3(0.0);
        extraLight.diffuse = u_ExtraLightColors[i];
        extraLight.specular = u_ExtraLightColors[i];

        result += CalcPointLight(extraLight, normal, v_FragPosWorld, viewDir, albedo);
    }

    // Environment reflection is blended after lighting and faded out for
    // shadowed fragments, which keep only their ambient contribution.
    vec3 environment = EnvironmentColor(normal, viewDir);
    float reflectionWeight = u_Reflectivity * (1.0 - shadow);

    FragColor = vec4(mix(result, environment, reflectionWeight), 1.0);
}
