#version 330 core

// Full-screen triangle generated from gl_VertexID; no vertex buffer needed.
void main()
{
    vec2 pos = vec2(float((gl_VertexID << 1) & 2),
                    float(gl_VertexID & 2));
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
