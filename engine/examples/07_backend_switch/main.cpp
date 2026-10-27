#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/RenderCommand.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/ShaderLibrary.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kVertices[] = {
    -0.6f, -0.5f, 0.0f,
     0.6f, -0.5f, 0.0f,
     0.0f,  0.6f, 0.0f,
};

void LogStats(const char* label)
{
    const RendererStats& stats = Renderer::GetStats();
    LOG_INFO(label << " stats | clear=" << stats.ClearCalls
                    << " draws=" << stats.DrawCalls
                    << " drawArrays=" << stats.DrawArraysCalls
                    << " indices=" << stats.IndexCount
                    << " vertices=" << stats.VertexCount);
}
} // namespace

int main()
{
    try
    {
        Window window(900, 600, "07_backend_switch - OpenGL", true, 4);

        const std::string assetDir = ASSET_DIR;

        ShaderLibrary shaderLibrary;
        std::shared_ptr<Shader> shader = shaderLibrary.Load(assetDir + "/shaders/backend.glsl");

        VertexBuffer vertexBuffer(kVertices, sizeof(kVertices));

        const VertexAttribute attributes[] = {
            {3, GL_FLOAT, false, 3 * sizeof(float), 0},
        };

        VertexArray vertexArray;
        vertexArray.Bind();
        vertexArray.AddVertexBuffer(vertexBuffer, attributes, 1);
        vertexArray.Unbind();

        GLFWwindow* nativeWindow = window.GetNativeHandle();

        window.SetKeyCallback([&](int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            if (action != GLFW_PRESS)
            {
                return;
            }

            if (key == GLFW_KEY_ESCAPE)
            {
                glfwSetWindowShouldClose(nativeWindow, GLFW_TRUE);
            }
            else if (key == GLFW_KEY_B)
            {
                const bool toNull = Renderer::GetBackend() == RendererBackend::OpenGL;
                Renderer::SetBackend(toNull ? RendererBackend::Null : RendererBackend::OpenGL);

                const char* backendName = toNull ? "Null" : "OpenGL";
                const std::string title = std::string("07_backend_switch - ") + backendName;
                glfwSetWindowTitle(nativeWindow, title.c_str());

                LogStats(backendName);
            }
            else if (key == GLFW_KEY_P)
            {
                LogStats("current backend");
            }
        });

        LOG_INFO("07_backend_switch controls: B toggle backend, P print stats, ESC quit");
        LogStats("OpenGL");

        float lastFrame = static_cast<float>(glfwGetTime());

        while (!window.ShouldClose())
        {
            const float currentFrame = static_cast<float>(glfwGetTime());
            const float deltaTime = currentFrame - lastFrame;
            (void)deltaTime;
            lastFrame = currentFrame;

            window.PollEvents();

            RenderCommand::Clear(0.09f, 0.1f, 0.14f, 1.0f);

            shader->Bind();
            shader->SetUniform("uTime", currentFrame * 0.8f);
            shader->SetUniform("uColor", glm::vec4(0.5f + 0.5f * std::sin(currentFrame * 1.3f),
                                                   0.5f + 0.5f * std::sin(currentFrame * 1.7f + 2.0f),
                                                   0.8f,
                                                   1.0f));
            RenderCommand::DrawArrays(vertexArray, *shader, GL_TRIANGLES, 3);

            window.SwapBuffers();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(e.what());
        return 1;
    }

    return 0;
}
