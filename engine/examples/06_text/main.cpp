#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <exception>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/TextRenderer.h"
#include "engine/Renderer/Texture.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kVertices[] = {
    // positions           // tex coords
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f,

     0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

     0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 1.0f,
};

const unsigned int kIndices[] = {
     0,  1,  2,  2,  3,  0,
     4,  5,  6,  6,  7,  4,
     8,  9, 10, 10, 11,  8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
};
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "06_text - Text Overlay", true, 4);

        const std::string assetDir = ASSET_DIR;
        Shader shader(assetDir + "/shaders/cube.vert",
                      assetDir + "/shaders/cube.frag");
        Texture texture(assetDir + "/checker.png");
        TextRenderer textRenderer;

        VertexBuffer vertexBuffer(kVertices, sizeof(kVertices));
        IndexBuffer indexBuffer(kIndices, sizeof(kIndices));

        const VertexAttribute attributes[] = {
            {3, GL_FLOAT, false, 5 * sizeof(float), 0},
            {2, GL_FLOAT, false, 5 * sizeof(float), 3 * sizeof(float)},
        };

        VertexArray vertexArray;
        vertexArray.Bind();
        vertexArray.AddVertexBuffer(vertexBuffer, attributes, 2);

        vertexArray.Bind();
        indexBuffer.Bind();
        vertexArray.Unbind();

        GLCall(glEnable(GL_DEPTH_TEST));

        Camera camera(glm::vec3(0.0f, 0.5f, 3.0f), -90.0f, -5.0f);

        GLFWwindow* nativeWindow = window.GetNativeHandle();
        glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        bool showText = true;

        window.SetKeyCallback([&](int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
            {
                glfwSetWindowShouldClose(nativeWindow, GLFW_TRUE);
            }
            if (action == GLFW_PRESS && key == GLFW_KEY_T)
            {
                showText = !showText;
            }
        });

        bool firstMouse = true;
        double lastX = 0.0;
        double lastY = 0.0;

        window.SetCursorPosCallback([&](double xPos, double yPos)
        {
            if (firstMouse)
            {
                lastX = xPos;
                lastY = yPos;
                firstMouse = false;
            }

            const float offsetX = static_cast<float>(xPos - lastX);
            const float offsetY = static_cast<float>(lastY - yPos);
            lastX = xPos;
            lastY = yPos;

            camera.ProcessMouse(offsetX, offsetY);
        });

        window.SetScrollCallback([&](double xOffset, double yOffset)
        {
            (void)xOffset;
            camera.ProcessMouseScroll(static_cast<float>(yOffset));
        });

        float lastFrame = static_cast<float>(glfwGetTime());
        float fpsTimer = 0.0f;
        int framesSinceFpsUpdate = 0;
        float framesPerSecond = 0.0f;

        while (!window.ShouldClose())
        {
            const float currentFrame = static_cast<float>(glfwGetTime());
            const float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            window.PollEvents();

            if (glfwGetKey(nativeWindow, GLFW_KEY_W) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Forward);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_S) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Backward);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_A) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Left);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_D) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Right);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_E) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Up);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_Q) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Down);
            }

            fpsTimer += deltaTime;
            ++framesSinceFpsUpdate;
            if (fpsTimer >= 0.5f)
            {
                framesPerSecond = static_cast<float>(framesSinceFpsUpdate) / fpsTimer;
                fpsTimer = 0.0f;
                framesSinceFpsUpdate = 0;
            }

            Renderer::Clear(0.08f, 0.09f, 0.12f, 1.0f);

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                                currentFrame * 0.6f,
                                                glm::vec3(0.4f, 1.0f, 0.2f));
            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()));

            shader.Bind();
            shader.SetUniform("uModel", model);
            shader.SetUniform("uView", view);
            shader.SetUniform("uProjection", projection);
            shader.SetUniform("uTexture", 0);

            texture.Bind(0);
            Renderer::Draw(vertexArray, indexBuffer, shader);

            if (showText)
            {
                char line[256];

                std::snprintf(line, sizeof(line), "FPS: %.1f", framesPerSecond);
                textRenderer.Draw(line, 16.0f, 18.0f, 2.0f,
                                  glm::vec4(1.0f, 0.85f, 0.3f, 1.0f),
                                  window.GetWidth(), window.GetHeight());

                std::snprintf(line, sizeof(line), "Resolution: %d x %d   MSAA: %dx",
                              window.GetWidth(), window.GetHeight(), window.GetMSAASamples());
                textRenderer.Draw(line, 16.0f, 48.0f, 2.0f,
                                  glm::vec4(0.9f, 0.95f, 1.0f, 1.0f),
                                  window.GetWidth(), window.GetHeight());

                std::snprintf(line, sizeof(line), "Backend: %s   Elapsed: %.1f s",
                              Renderer::GetBackend() == RendererBackend::OpenGL ? "OpenGL" : "Null",
                              currentFrame);
                textRenderer.Draw(line, 16.0f, 78.0f, 2.0f,
                                  glm::vec4(0.6f, 0.9f, 0.7f, 1.0f),
                                  window.GetWidth(), window.GetHeight());

                textRenderer.Draw("WASD move  Q/E down/up  mouse look  wheel zoom  T toggle text  ESC quit",
                                  16.0f, 116.0f, 1.5f,
                                  glm::vec4(0.75f, 0.78f, 0.85f, 1.0f),
                                  window.GetWidth(), window.GetHeight());
            }

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
