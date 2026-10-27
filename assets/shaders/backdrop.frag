#version 330 core

out vec4 FragColor;

uniform vec2 u_Resolution;
uniform float u_Time;

float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / max(u_Resolution, vec2(1.0));
    vec2 p = uv * 2.0 - 1.0;
    p.x *= u_Resolution.x / u_Resolution.y;

    // Warm pool of light behind the flower.
    vec2 glowCenter = vec2(0.0, 0.12);
    float glowRadius = length(p - glowCenter);
    float glow = exp(-glowRadius * glowRadius * 2.4);

    vec3 topColor = vec3(0.095, 0.038, 0.055);
    vec3 bottomColor = vec3(0.010, 0.007, 0.012);

    vec3 color = mix(bottomColor, topColor, smoothstep(-0.9, 0.9, p.y));
    color += vec3(0.24, 0.050, 0.070) * glow * 0.75;

    // A faint drifting haze keeps the backdrop from looking perfectly flat.
    float haze =
        0.5 + 0.5 * sin(p.x * 2.7 + u_Time * 0.12) *
                        sin(p.y * 3.1 - u_Time * 0.09);
    color += vec3(0.020, 0.012, 0.018) * haze;

    // Vignette.
    float vignette =
        smoothstep(1.55, 0.25, length(p * vec2(0.85, 1.0)));
    color *= mix(0.50, 1.0, vignette);

    // Very light grain to avoid colour banding on the gradient.
    color += (Hash(gl_FragCoord.xy) - 0.5) * 0.012;

    FragColor = vec4(color, 1.0);
}
