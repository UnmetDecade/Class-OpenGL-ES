#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

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
uniform vec3 uViewPos;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return (ambient + diffuse + specular) * albedo;
}

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

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 albedo = texture(uTexture, vTexCoord).rgb;

    vec3 result = CalcDirLight(uDirLight, normal, viewDir, albedo);
    result += CalcPointLight(uPointLight, normal, vFragPos, viewDir, albedo);

    FragColor = vec4(result, 1.0);
}
