#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

// 0 = petals, 1 = stem/receptacle, 2 = leaves/sepals
uniform int uMode;

uniform vec3 uViewPos;
uniform vec3 uHeadCenter;
uniform float uTime;

uniform vec3 uPetalDeep;
uniform vec3 uPetalMid;
uniform vec3 uPetalBright;
uniform vec3 uPetalRim;

uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uShadowBias;
uniform int uUseShadows;

// Direction *towards* the light.
#define KEY_DIR normalize(vec3(-0.55, 0.95, 0.65))
#define FILL_DIR normalize(vec3(0.85, 0.20, 0.35))
#define RIM_DIR normalize(vec3(0.15, 0.35, -1.00))

#define KEY_COLOR (vec3(1.00, 0.90, 0.78) * 1.30)
#define FILL_COLOR (vec3(0.35, 0.48, 0.78) * 0.18)
#define RIM_COLOR (vec3(1.00, 0.14, 0.16) * 0.90)

float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float ValueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = Hash(i);
    float b = Hash(i + vec2(1.0, 0.0));
    float c = Hash(i + vec2(0.0, 1.0));
    float d = Hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Percentage-closer filtered shadows from the key light. Petals are thin,
// double-sided surfaces, so both a normal offset and a slope-scaled depth bias
// are needed to keep them from shadowing themselves.
float ShadowFactor(vec3 worldPos, vec3 N, float ndl)
{
    if (uUseShadows == 0)
    {
        return 1.0;
    }

    vec3 offsetPos = worldPos + N * (0.006 + 0.012 * (1.0 - ndl));
    vec4 lightPos = uLightSpaceMatrix * vec4(offsetPos, 1.0);
    vec3 proj = lightPos.xyz / lightPos.w;
    proj = proj * 0.5 + 0.5;

    if (proj.x <= 0.0 || proj.x >= 1.0 || proj.y <= 0.0 || proj.y >= 1.0 ||
        proj.z >= 1.0)
    {
        return 1.0;
    }

    float bias = uShadowBias * (1.0 + 2.5 * (1.0 - ndl));
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));

    // A 5x5 box, slightly wider than one texel, softens the self-shadowing
    // so the deep folds between petals read as penumbra rather than cut-outs.
    float lit = 0.0;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float closest =
                texture(uShadowMap, proj.xy + vec2(x, y) * texel * 1.6).r;
            lit += (proj.z - bias > closest) ? 0.0 : 1.0;
        }
    }

    return lit / 25.0;
}

vec3 PetalAlbedo(float s, float t)
{
    // How far this fragment sits from the flower axis (the turntable rotates
    // around Y, so this distance is invariant and usable for shading).
    float radial = length(vWorldPos.xz - uHeadCenter.xz);
    float openness = smoothstep(0.02, 0.42, radial);

    // Petal tips and outer whorls are brighter; petal bases and the depths of
    // the bud stay dark and saturated.
    float grad = clamp(s * 0.62 + openness * 0.62, 0.0, 1.0);

    vec3 color = mix(uPetalDeep, uPetalMid, smoothstep(0.0, 0.58, grad));
    color = mix(color, uPetalBright, smoothstep(0.58, 1.0, grad));

    // Soft light rim along the petal edges near the tip.
    float edge = smoothstep(0.55, 1.0, abs(t));
    color = mix(color, uPetalRim, edge * 0.30 * smoothstep(0.30, 1.0, s));

    // Central vein plus faint radiating veins.
    float vein = exp(-pow(t * 6.5, 2.0));
    color *= 1.0 + 0.06 * vein;

    float veins =
        0.5 + 0.5 * sin((t * (7.0 + 4.0 * s) + 0.35 * sin(s * 9.0)) * 6.28318);
    color *= 1.0 - 0.035 * smoothstep(0.70, 1.0, veins) * s;

    // Subtle mottling so large petals do not look like flat plastic.
    float mottling =
        ValueNoise(vec2(t * 5.0 + s * 2.7, s * 8.0 + uTime * 0.03));
    color *= 0.92 + 0.16 * mottling;

    return color;
}

vec3 ShadePetals(vec3 albedo, vec3 N, vec3 V)
{
    // Wrapped diffuse reads as the soft, velvety scattering of rose petals.
    float ndl = dot(N, KEY_DIR);
    float lambert = max(ndl, 0.0);
    float wrap = clamp((ndl + 0.45) / 1.45, 0.0, 1.0);
    float shadow = ShadowFactor(vWorldPos, N, lambert);

    vec3 diffuse = KEY_COLOR * (0.35 * lambert + 0.65 * wrap) * shadow;
    diffuse += FILL_COLOR * max(dot(N, FILL_DIR), 0.0) * 0.75;

    vec3 ambient =
        mix(vec3(0.022, 0.005, 0.010), vec3(0.085, 0.028, 0.042),
            N.y * 0.5 + 0.5);

    // Fake subsurface scattering: only petals whose back faces the rim light
    // glow, which keeps front-lit petals deep and saturated.
    float through = pow(max(dot(-N, RIM_DIR), 0.0), 1.2);
    float viewAlign = pow(max(dot(V, -RIM_DIR), 0.0), 2.0);
    vec3 transmitted = albedo * vec3(1.15, 0.12, 0.16) *
                       (through * viewAlign * 0.32) * (0.35 + 0.65 * shadow);

    // Two-lobe sheen, tinted by the petal so highlights stay crimson instead
    // of washing out to white-pink.
    vec3 halfVector = normalize(KEY_DIR + V);
    float ndh = max(dot(N, halfVector), 0.0);
    float specular = pow(ndh, 48.0) * 0.075 + pow(ndh, 10.0) * 0.012;
    vec3 specularTint = mix(vec3(1.0), albedo * 3.5, 0.85);

    // Fresnel rim picks out the silhouette against the dark background.
    float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);

    vec3 color = albedo * (ambient + diffuse) + transmitted +
                 KEY_COLOR * specularTint * specular *
                     (0.20 + 0.80 * shadow) +
                 RIM_COLOR * fresnel * 0.12;

    // Occlusion deep inside the flower.  The central bud is excluded so it
    // reads as a lit furled heart instead of a black cavity.
    if (uMode != 0)
    {
        return color;
    }

    float radial = length(vWorldPos.xz - uHeadCenter.xz);
    float innerAo = mix(0.35, 1.0, smoothstep(0.04, 0.30, radial));
    float petalAo = 0.55 + 0.45 * smoothstep(0.0, 0.35, vTexCoord.x);
    return color * petalAo * innerAo;
}

vec3 StemAlbedo(float u)
{
    vec3 color = mix(vec3(0.024, 0.060, 0.017),
                     vec3(0.045, 0.105, 0.030), u);
    color *= 0.90 + 0.20 * ValueNoise(vec2(vTexCoord.y * 9.0, u * 28.0));
    return color;
}

vec3 LeafAlbedo(float s, float t)
{
    vec3 color = mix(vec3(0.020, 0.052, 0.014), vec3(0.055, 0.155, 0.032),
                     smoothstep(0.0, 1.0, s));

    // Midrib.
    float midrib = exp(-pow(t * 12.0, 2.0));
    color = mix(color, vec3(0.10, 0.24, 0.06), midrib * 0.50);

    // Lateral veins fanning out from the midrib.
    float lateral = 0.5 + 0.5 * sin((abs(t) * 10.0 + s * 14.0) * 2.0);
    color *= 1.0 - 0.06 * smoothstep(0.80, 1.0, lateral) * s;

    color *= 1.0 - 0.15 * smoothstep(0.75, 1.0, abs(t));

    float mottling = ValueNoise(vec2(t * 8.0, s * 12.0));
    color *= 0.90 + 0.20 * mottling;

    return color;
}

vec3 ShadeFoliage(vec3 albedo, vec3 N, vec3 V, float translucency)
{
    float ndl = dot(N, KEY_DIR);
    float lambert = max(ndl, 0.0);
    float wrap = clamp((ndl + 0.35) / 1.35, 0.0, 1.0);
    float shadow = ShadowFactor(vWorldPos, N, lambert);

    vec3 diffuse = KEY_COLOR * (0.40 * lambert + 0.60 * wrap) * shadow +
                   FILL_COLOR * max(dot(N, FILL_DIR), 0.0) * 0.90;

    vec3 ambient =
        mix(vec3(0.015, 0.020, 0.012), vec3(0.080, 0.100, 0.080),
            N.y * 0.5 + 0.5);

    float back = pow(max(dot(V, -RIM_DIR), 0.0), 2.0) *
                 pow(max(dot(-N, RIM_DIR), 0.0), 1.5);
    vec3 transmitted = albedo * vec3(0.55, 1.00, 0.35) * back * translucency *
                       (0.45 + 0.55 * shadow);

    vec3 halfVector = normalize(KEY_DIR + V);
    float ndh = max(dot(N, halfVector), 0.0);
    float specular = pow(ndh, 64.0) * 0.10 + pow(ndh, 12.0) * 0.02;

    float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);

    return albedo * (ambient + diffuse) + transmitted +
           KEY_COLOR * specular * (0.25 + 0.75 * shadow) +
           FILL_COLOR * fresnel * 0.18;
}

vec3 ACESFilm(vec3 x)
{
    return clamp((x * (2.51 * x + 0.03)) /
                     (x * (2.43 * x + 0.59) + 0.14),
                 0.0, 1.0);
}

void main()
{
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing)
    {
        N = -N; // petals and leaves are single-sided surfaces
    }

    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 color;
    if (uMode == 0)
    {
        vec3 albedo = PetalAlbedo(vTexCoord.x, vTexCoord.y);
        color = ShadePetals(albedo, N, V);
    }
    else if (uMode == 1)
    {
        vec3 albedo = StemAlbedo(vTexCoord.x);
        color = ShadeFoliage(albedo, N, V, 0.60);
    }
    else if (uMode == 3)
    {
        // The furled heart: slightly warmer and brighter than the deep petals
        // so the centre of the bloom stays readable.
        vec3 albedo = mix(uPetalMid, uPetalBright, 0.42);
        albedo *= 1.0 + 0.18 * ValueNoise(vTexCoord * 9.0);
        color = ShadePetals(albedo, N, V);
    }
    else
    {
        vec3 albedo = LeafAlbedo(vTexCoord.x, vTexCoord.y);
        color = ShadeFoliage(albedo, N, V, 1.20);
    }

    // Push the petals towards a richer crimson without crushing the foliage.
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.10);

    color = ACESFilm(color * 1.08);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
