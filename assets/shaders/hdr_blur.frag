#version 330 core

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;
uniform int u_Horizontal;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_ScreenTexture, 0));
    vec2 direction = (u_Horizontal != 0) ? vec2(texelSize.x, 0.0)
                                         : vec2(0.0, texelSize.y);

    // 9-tap Gaussian, run twice (horizontal then vertical) on a half-resolution
    // target to keep the bloom cheap.
    const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    vec3 result = texture(u_ScreenTexture, v_TexCoords).rgb * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        result += texture(u_ScreenTexture, v_TexCoords + direction * float(i)).rgb * weights[i];
        result += texture(u_ScreenTexture, v_TexCoords - direction * float(i)).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
