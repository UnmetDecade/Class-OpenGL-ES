#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_ScreenTexture, 0));
    vec4 result = vec4(0.0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            result += texture(u_ScreenTexture, v_TexCoords + vec2(x, y) * texelSize);
        }
    }

    FragColor = result / 9.0;
}
