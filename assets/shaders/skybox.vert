#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 v_TexCoords;

uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    v_TexCoords = aPos;

    // Remove the view translation and force z = w so the cube is always at
    // the far plane (used together with glDepthFunc(GL_LEQUAL)).
    vec4 position = u_Projection * mat4(mat3(u_View)) * vec4(aPos, 1.0);
    gl_Position = position.xyww;
}
