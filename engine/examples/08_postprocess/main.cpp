#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <exception>
#include <memory>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Framebuffer.h"
#include "engine/Renderer/PostEffect.h"
#include "engine/Renderer/PostProcess.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/Texture.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kCubeVertices[] = {
    // positions           // tex coords
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,   // front
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f,

     0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   // back
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   // left
    -0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

     0.5f, -0.5f,  0.5f,   0.0f, 0.0f,   // right
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f,   // top
     0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   // bottom
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 1.0f,
};

const unsigned int kCubeIndices[] = {
     0,  1,  2,  2,  3,  0,
     4,  5,  6,  6,  7,  4,
     8,  9, 10, 10, 11,  8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
};

const float kScreenQuadVertices[] = {
    // positions   // tex coords
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
    -1.0f,  1.0f,   0.0f, 1.0f,
};
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "08_postprocess - Stage 8");

        const std::string assetDir = ASSET_DIR;
        const std::string postVertPath = assetDir + "/shaders/postprocess.vert";

        Shader cubeShader(assetDir + "/shaders/cube.vert",
                          assetDir + "/shaders/cube.frag");
        // Final output pass: LDR branch with the explicit linear -> sRGB
        // encode (u_EncodeLDR), since the scene targets hold linear values.
        Shader screenShader(postVertPath, assetDir + "/shaders/hdr_output.frag");

        VertexBuffer cubeVertexBuffer(kCubeVertices, sizeof(kCubeVertices));
        IndexBuffer cubeIndexBuffer(kCubeIndices, sizeof(kCubeIndices));

        const VertexAttribute cubeAttributes[] = {
            {3, GL_FLOAT, false, 5 * sizeof(float), 0},
            {2, GL_FLOAT, false, 5 * sizeof(float), 3 * sizeof(float)},
        };

        VertexArray cubeVertexArray;
        cubeVertexArray.Bind();
        cubeVertexArray.AddVertexBuffer(cubeVertexBuffer, cubeAttributes, 2);
        cubeVertexArray.Bind();
        cubeIndexBuffer.Bind();
        cubeVertexArray.Unbind();

        // Full-screen quad used to present the final post-processed texture.
        VertexBuffer screenVertexBuffer(kScreenQuadVertices, sizeof(kScreenQuadVertices));
        const VertexAttribute screenAttributes[] = {
            {2, GL_FLOAT, false, 4 * sizeof(float), 0},
            {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
        };

        VertexArray screenVertexArray;
        screenVertexArray.Bind();
        screenVertexArray.AddVertexBuffer(screenVertexBuffer, screenAttributes, 2);
        screenVertexArray.Unbind();

        Texture texture(assetDir + "/container.jpg");
        Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

        PostProcess postProcess(static_cast<unsigned int>(window.GetWidth()),
                                static_cast<unsigned int>(window.GetHeight()));

        const auto makeEffectShader = [&](const char* fragmentFile)
        {
            return std::make_shared<Shader>(postVertPath, assetDir + "/shaders/" + fragmentFile);
        };

        // Insertion order is the execution order; the Gray + Edge combo runs
        // Gray first, then Edge.
        postProcess.AddEffect(std::make_unique<ShaderPostEffect>("Gray"),
                              makeEffectShader("post_gray.frag"));
        postProcess.AddEffect(std::make_unique<ShaderPostEffect>("Invert"),
                              makeEffectShader("post_invert.frag"));
        postProcess.AddEffect(std::make_unique<ShaderPostEffect>("Blur"),
                              makeEffectShader("post_blur.frag"));
        postProcess.AddEffect(std::make_unique<ShaderPostEffect>("Edge"),
                              makeEffectShader("post_edge.frag"));

        const auto selectEffect = [&postProcess](const std::string& name)
        {
            postProcess.SetEffectEnabled("Gray", name == "Gray");
            postProcess.SetEffectEnabled("Invert", name == "Invert");
            postProcess.SetEffectEnabled("Blur", name == "Blur");
            postProcess.SetEffectEnabled("Edge", name == "Edge");

            LOG_INFO("Post-process effect: " << (name.empty() ? "None" : name));
        };

        // Stage 8 acceptance: the default picture must match the direct cube
        // render, so start with the pass-through chain (all effects off).
        selectEffect("");

        GLFWwindow* nativeWindow = window.GetNativeHandle();
        glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        window.SetKeyCallback([&](int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            if (action != GLFW_PRESS)
            {
                return;
            }

            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(nativeWindow, GLFW_TRUE);
                break;
            case GLFW_KEY_1:
                selectEffect("");
                break;
            case GLFW_KEY_2:
                selectEffect("Gray");
                break;
            case GLFW_KEY_3:
                selectEffect("Invert");
                break;
            case GLFW_KEY_4:
                selectEffect("Edge");
                break;
            case GLFW_KEY_5:
                selectEffect("Blur");
                break;
            case GLFW_KEY_6:
                selectEffect("");
                postProcess.SetEffectEnabled("Gray", true);
                postProcess.SetEffectEnabled("Edge", true);
                LOG_INFO("Post-process effect: Gray + Edge");
                break;
            default:
                break;
            }
        });

        window.SetFramebufferSizeCallback([&postProcess](int width, int height)
        {
            if (width > 0 && height > 0)
            {
                postProcess.Resize(static_cast<unsigned int>(width),
                                   static_cast<unsigned int>(height));
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

            // --- Scene pass: render into the off-screen scene framebuffer ---
            postProcess.BeginScene();
            Renderer::Clear(0.1f, 0.15f, 0.25f, 1.0f);

            const int windowWidth = window.GetWidth();
            const int windowHeight = window.GetHeight();
            const float aspect = (windowHeight > 0)
                                     ? static_cast<float>(windowWidth) / static_cast<float>(windowHeight)
                                     : 16.0f / 9.0f;

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                                currentFrame,
                                                glm::vec3(0.5f, 1.0f, 0.0f));
            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(aspect);

            cubeShader.Bind();
            cubeShader.SetUniform("uModel", model);
            cubeShader.SetUniform("uView", view);
            cubeShader.SetUniform("uProjection", projection);
            cubeShader.SetUniform("uTexture", 0);

            texture.Bind(0);
            Renderer::Draw(cubeVertexArray, cubeIndexBuffer, cubeShader);
            postProcess.EndScene();

            // --- Post-processing chain + presentation to the default FBO ---
            const unsigned int finalTexture = postProcess.Render();

            GLCall(glViewport(0, 0, windowWidth, windowHeight));
            GLCall(glDisable(GL_DEPTH_TEST));

            screenShader.Bind();
            screenShader.SetUniform("u_ScreenTexture", 0);
            screenShader.SetUniform("u_BloomTexture", 1);
            screenShader.SetUniform("u_UseHDR", 0);
            screenShader.SetUniform("u_EncodeLDR", 1);
            screenShader.SetUniform("u_UseBloom", 0);

            GLCall(glActiveTexture(GL_TEXTURE0));
            GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
            // Bloom is unused here, but a sampler still needs a bound texture.
            GLCall(glActiveTexture(GL_TEXTURE1));
            GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));

            screenVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
            screenVertexArray.Unbind();

            screenShader.Unbind();

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
