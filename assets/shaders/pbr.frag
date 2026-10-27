#version 330 core

in vec3 v_FragPosWorld;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

const float PI = 3.14159265359;
const int MAX_POINT_LIGHTS = 8;

struct Material
{
    vec3 Albedo;
    float Metallic;
    float Roughness;
    float AO;
};

uniform Material u_Material;

// Optional albedo texture (used by the checker ground).
uniform sampler2D u_AlbedoMap;
uniform int u_UseAlbedoMap;

// Point lights: color already carries the HDR intensity (components may be > 1).
uniform int u_NumPointLights;
uniform vec3 u_PointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 u_PointLightColors[MAX_POINT_LIGHTS];
uniform float u_PointLightRadius[MAX_POINT_LIGHTS];

// Directional light: u_DirLightDirection is the direction the light travels.
uniform vec3 u_DirLightDirection;
uniform vec3 u_DirLightColor;

uniform vec3 uViewPos;

uniform vec3 u_AmbientColor;
uniform bool u_LambertOnly;

uniform sampler2D u_ShadowMap;
uniform mat4 u_LightSpaceMatrix;
uniform float u_BiasScale;
uniform int u_UsePCF;

uniform samplerCube u_IrradianceMap;
uniform int u_UseIBL;

// ---------------------------------------------------------------------------
// Cook-Torrance BRDF
// ---------------------------------------------------------------------------

// Trowbridge-Reitz (GGX) normal distribution:
//   D = a^2 / (PI * ((NdotH^2 * (a^2 - 1) + 1)^2)),  a = roughness^2
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-7);
}

// Schlick-GGX geometry term for one direction:
//   G1 = NdotX / (NdotX * (1 - k) + k),  k = (roughness + 1)^2 / 8
float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-7);
}

// Smith's method: the full geometry term is the product of both directions.
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

// Schlick Fresnel: F0 + (1 - F0) * (1 - VdotH)^5
vec3 FresnelSchlick(float VdotH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// ---------------------------------------------------------------------------
// Shadows (same manual depth comparison as phase 9)
// ---------------------------------------------------------------------------

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

// Inverse-square falloff with a smooth window that reaches zero at the light's
// radius, so the finite light volume has no hard edge.
float PointAttenuation(float distance, float radius)
{
    float ratio = clamp(distance / max(radius, 1e-3), 0.0, 1.0);
    float window = 1.0 - ratio * ratio * ratio * ratio;
    window = clamp(window, 0.0, 1.0);
    return (window * window) / max(distance * distance, 0.01);
}

// ---------------------------------------------------------------------------
// Direct lighting
// ---------------------------------------------------------------------------

// Evaluates one light for a metallic-roughness surface:
//   kS = F
//   kD = (1 - kS) * (1 - metallic)
//   Lo = (kD * albedo / PI + D * G * F / (4 * NdotV * NdotL))
//        * radiance * NdotL
vec3 DirectLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, vec3 F0)
{
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float roughness = clamp(u_Material.Roughness, 0.04, 1.0);

    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    vec3 F = FresnelSchlick(VdotH, F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - u_Material.Metallic);

    vec3 diffuse = kD * albedo / PI;
    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);

    // Debug comparison: keep only the diffuse (kd) term.
    if (u_LambertOnly)
    {
        specular = vec3(0.0);
    }

    return (diffuse + specular) * radiance * NdotL;
}

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(uViewPos - v_FragPosWorld);

    vec3 albedo = max(u_Material.Albedo, vec3(0.0));
    if (u_UseAlbedoMap != 0)
    {
        albedo *= texture(u_AlbedoMap, v_TexCoord).rgb;
    }

    float metallic = clamp(u_Material.Metallic, 0.0, 1.0);
    // F0 = mix(0.04, albedo, metallic): dielectric base reflectance -> metal tint.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Directional light (shadow factor modulates direct light only) ---
    vec3 L = normalize(-u_DirLightDirection);
    float shadow = ShadowCalculation(N, L);
    vec3 Lo = DirectLight(N, V, L, u_DirLightColor * (1.0 - shadow), albedo, F0);

    // --- Point lights, accumulated in a single pass ---
    for (int i = 0; i < u_NumPointLights; ++i)
    {
        vec3 toLight = u_PointLightPositions[i] - v_FragPosWorld;
        float distance = length(toLight);
        if (distance <= 1e-4)
        {
            continue;
        }

        L = toLight / distance;
        float attenuation = PointAttenuation(distance, u_PointLightRadius[i]);
        Lo += DirectLight(N, V, L,
                          u_PointLightColors[i] * attenuation,
                          albedo, F0);
    }

    // --- Ambient: diffuse irradiance (optional IBL) or a constant fallback ---
    vec3 ambient;
    if (u_UseIBL != 0)
    {
        ambient = texture(u_IrradianceMap, N).rgb * albedo * u_Material.AO;
    }
    else
    {
        ambient = albedo * u_AmbientColor * u_Material.AO;
    }

    FragColor = vec4(ambient + Lo, 1.0);
}
