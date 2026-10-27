#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;

void main()
{
    vec4 color = texture(u_ScreenTexture, v_TexCoords);
    FragColor = vec4(vec3(1.0) - color.rgb, color.a);
}
