#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;

void main()
{
    FragColor = texture(u_ScreenTexture, v_TexCoords);
}
