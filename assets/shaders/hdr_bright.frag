#version 330 core

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;
uniform float u_Threshold;

void main()
{
    vec3 color = texture(u_ScreenTexture, v_TexCoords).rgb;

    // Hard threshold (no soft knee): keep only HDR pixels brighter than the
    // threshold so the bloom pass starts from the brightest regions.
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    FragColor = vec4(brightness > u_Threshold ? color : vec3(0.0), 1.0);
}
