#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 v_FragPosWorld;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    v_FragPosWorld = worldPos.xyz;
    v_Normal = uNormalMatrix * aNormal;
    v_TexCoord = aTexCoord;

    gl_Position = uProjection * uView * worldPos;
}
