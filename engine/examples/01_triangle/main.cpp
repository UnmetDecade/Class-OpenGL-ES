#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <exception>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Shader.h"
#include "engine/Window/Window.h"

namespace
{
const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* kFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform float uTime;

void main()
{
    vec3 colorA = vec3(1.0, 0.5, 0.2);
    vec3 colorB = vec3(0.2, 0.6, 1.0);
    float t = 0.5 + 0.5 * sin(uTime * 2.0);
    FragColor = vec4(mix(colorA, colorB, t), 1.0);
}
)";

const float kVertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f,
};
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "01_triangle - Stage 1");

        Shader shader(kVertexShader, kFragmentShader);

        GLuint vao = 0;
        GLuint vbo = 0;
        GLCall(glGenVertexArrays(1, &vao));
        GLCall(glGenBuffers(1, &vbo));

        GLCall(glBindVertexArray(vao));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW));

        GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));
        GLCall(glEnableVertexAttribArray(0));

        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GLCall(glBindVertexArray(0));

        while (!window.ShouldClose())
        {
            window.PollEvents();

            GLCall(glClearColor(0.1f, 0.15f, 0.25f, 1.0f));
            GLCall(glClear(GL_COLOR_BUFFER_BIT));

            shader.Bind();
            shader.SetUniform("uTime", static_cast<float>(glfwGetTime()));

            GLCall(glBindVertexArray(vao));
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 3));

            shader.Unbind();
            window.SwapBuffers();
        }

        GLCall(glDeleteBuffers(1, &vbo));
        GLCall(glDeleteVertexArrays(1, &vao));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(e.what());
        return 1;
    }

    return 0;
}
