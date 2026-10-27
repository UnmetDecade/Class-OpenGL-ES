#type vertex
#version 330 core
layout(location = 0) in vec3 aPos;

uniform float uTime;

void main()
{
    float c = cos(uTime);
    float s = sin(uTime);
    mat2 rotation = mat2(c, -s, s, c);
    gl_Position = vec4(rotation * aPos.xy, aPos.z, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
