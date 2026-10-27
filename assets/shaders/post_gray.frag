#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;

void main()
{
    vec4 color = texture(u_ScreenTexture, v_TexCoords);
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), color.a);
}
