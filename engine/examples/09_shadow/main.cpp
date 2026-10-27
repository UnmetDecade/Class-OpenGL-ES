#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>
#include <vector>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Framebuffer.h"
#include "engine/Renderer/Lighting.h"
#include "engine/Renderer/Mesh.h"
#include "engine/Renderer/Model.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/ShadowMap.h"
#include "engine/Renderer/Texture.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kGroundY = -0.4f;

const std::vector<Vertex> kGroundVertices = {
    {{-10.0f, kGroundY, -10.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-10.0f, kGroundY,  10.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 8.0f}},
    {{ 10.0f, kGroundY,  10.0f}, {0.0f, 1.0f, 0.0f}, {8.0f, 8.0f}},
    {{ 10.0f, kGroundY, -10.0f}, {0.0f, 1.0f, 0.0f}, {8.0f, 0.0f}},
};

const std::vector<unsigned int> kGroundIndices = {0, 1, 2, 0, 2, 3};

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
        Window window(1280, 720, "09_shadow - Shadow Mapping", true, 4);

        const std::string assetDir = ASSET_DIR;

        Shader shader(assetDir + "/shaders/lighting_shadow.vert",
                      assetDir + "/shaders/lighting_shadow.frag");
        Shader shadowDepthShader(assetDir + "/shaders/shadow_depth.vert",
                                 assetDir + "/shaders/shadow_depth.frag");
        // Final output pass: LDR branch with the explicit linear -> sRGB
        // encode, since the off-screen scene target holds linear values.
        Shader screenShader(assetDir + "/shaders/postprocess.vert",
                            assetDir + "/shaders/hdr_output.frag");

        Model torus(assetDir + "/models/torus.obj");
        Mesh ground(kGroundVertices, kGroundIndices);
        Texture checker(assetDir + "/checker.png");

        ShadowMap shadowMap(2048);

        // Off-screen scene target so the frame can be presented through the
        // shared output shader (single linear -> sRGB encode) instead of
        // relying on default-framebuffer sRGB support.
        Framebuffer sceneFramebuffer(static_cast<unsigned int>(window.GetWidth()),
                                     static_cast<unsigned int>(window.GetHeight()));

        VertexBuffer screenVertexBuffer(kScreenQuadVertices, sizeof(kScreenQuadVertices));
        const VertexAttribute screenAttributes[] = {
            {2, GL_FLOAT, false, 4 * sizeof(float), 0},
            {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
        };

        VertexArray screenVertexArray;
        screenVertexArray.Bind();
        screenVertexArray.AddVertexBuffer(screenVertexBuffer, screenAttributes, 2);
        screenVertexArray.Unbind();

        GLCall(glEnable(GL_DEPTH_TEST));

        Camera camera(glm::vec3(0.0f, 0.8f, 3.6f), -90.0f, -8.0f);

        Material material;
        material.ambient = glm::vec3(0.35f);
        material.diffuse = glm::vec3(0.9f);
        material.specular = glm::vec3(0.6f);
        material.shininess = 64.0f;

        DirectionalLight dirLight;
        dirLight.direction = glm::vec3(-0.7f, -0.7f, -0.35f);
        dirLight.ambient = glm::vec3(0.15f);
        dirLight.diffuse = glm::vec3(0.75f, 0.8f, 0.9f);
        dirLight.specular = glm::vec3(1.0f);

        PointLight pointLight;
        pointLight.constant = 1.0f;
        pointLight.linear = 0.35f;
        pointLight.quadratic = 0.44f;
        pointLight.diffuse = glm::vec3(1.0f, 0.55f, 0.25f);
        pointLight.specular = glm::vec3(1.0f);

        // Light frustum: an orthographic box aimed from the directional light
        // towards the scene center.
        const glm::vec3 lightDir = glm::normalize(dirLight.direction);
        const glm::vec3 lightPosition = -lightDir * 10.0f;
        const glm::mat4 lightView = glm::lookAt(lightPosition,
                                                glm::vec3(0.0f),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f,
                                                     -10.0f, 10.0f,
                                                     1.0f, 20.0f);
        const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        const glm::mat4 groundModel = glm::mat4(1.0f);
        const glm::mat3 groundNormalMatrix = glm::mat3(1.0f);

        float biasScale = 1.0f;
        int usePCF = 1;

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
            case GLFW_KEY_LEFT_BRACKET:
                biasScale = std::max(0.1f, biasScale - 0.1f);
                LOG_INFO("Shadow bias scale: " << biasScale);
                break;
            case GLFW_KEY_RIGHT_BRACKET:
                biasScale = std::min(5.0f, biasScale + 0.1f);
                LOG_INFO("Shadow bias scale: " << biasScale);
                break;
            case GLFW_KEY_F6:
                usePCF = usePCF != 0 ? 0 : 1;
                LOG_INFO("PCF 3x3: " << (usePCF != 0 ? "on" : "off"));
                break;
            default:
                break;
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

        window.SetFramebufferSizeCallback([&sceneFramebuffer](int width, int height)
        {
            if (width > 0 && height > 0)
            {
                sceneFramebuffer.Resize(static_cast<unsigned int>(width),
                                        static_cast<unsigned int>(height));
            }
        });

        LOG_INFO("09_shadow controls: WASD move, Q/E down/up, mouse look, wheel zoom, "
                 "[ / ] bias, F6 PCF, ESC quit");

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

            pointLight.position = glm::vec3(std::cos(currentFrame) * 2.0f,
                                            1.4f + std::sin(currentFrame * 0.7f) * 0.3f,
                                            std::sin(currentFrame) * 2.0f);

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                                currentFrame * 0.35f,
                                                glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

            // --- Shadow pass: depth from the light's point of view ---
            shadowMap.BeginWrite(lightSpaceMatrix);

            shadowDepthShader.Bind();
            shadowDepthShader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
            shadowDepthShader.SetUniform("u_Model", groundModel);
            ground.Draw(shadowDepthShader);

            shadowDepthShader.Bind();
            shadowDepthShader.SetUniform("u_Model", model);
            torus.Draw(shadowDepthShader);

            shadowMap.EndWrite(static_cast<unsigned int>(window.GetWidth()),
                               static_cast<unsigned int>(window.GetHeight()));

            // --- Main pass: render into the off-screen scene target ---
            sceneFramebuffer.Bind();
            GLCall(glViewport(0, 0,
                              static_cast<GLsizei>(window.GetWidth()),
                              static_cast<GLsizei>(window.GetHeight())));
            Renderer::Clear(0.05f, 0.06f, 0.09f, 1.0f);

            shadowMap.BindForReading(1);
            checker.Bind(0);

            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()));

            shader.Bind();
            shader.SetUniform("uView", view);
            shader.SetUniform("uProjection", projection);
            shader.SetUniform("uViewPos", camera.GetPosition());
            shader.SetUniform("uTexture", 0);
            shader.SetUniform("u_ShadowMap", 1);
            shader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
            shader.SetUniform("u_BiasScale", biasScale);
            shader.SetUniform("u_UsePCF", usePCF);

            material.Apply(shader);
            dirLight.Apply(shader);
            pointLight.Apply(shader);

            shader.SetUniform("uModel", groundModel);
            shader.SetUniform("uNormalMatrix", groundNormalMatrix);
            ground.Draw(shader);

            shader.Bind();
            shader.SetUniform("uModel", model);
            shader.SetUniform("uNormalMatrix", normalMatrix);
            torus.Draw(shader);

            // --- Final output pass: linear -> sRGB encode to the default FBO ---
            Framebuffer::Unbind();
            GLCall(glViewport(0, 0,
                              static_cast<GLsizei>(window.GetWidth()),
                              static_cast<GLsizei>(window.GetHeight())));
            GLCall(glDisable(GL_DEPTH_TEST));

            screenShader.Bind();
            screenShader.SetUniform("u_ScreenTexture", 0);
            screenShader.SetUniform("u_BloomTexture", 1);
            screenShader.SetUniform("u_UseHDR", 0);
            screenShader.SetUniform("u_EncodeLDR", 1);
            screenShader.SetUniform("u_UseBloom", 0);

            const unsigned int sceneTexture = sceneFramebuffer.GetColorAttachmentID();
            GLCall(glActiveTexture(GL_TEXTURE0));
            GLCall(glBindTexture(GL_TEXTURE_2D, sceneTexture));
            // Bloom is unused here, but a sampler still needs a bound texture.
            GLCall(glActiveTexture(GL_TEXTURE1));
            GLCall(glBindTexture(GL_TEXTURE_2D, sceneTexture));

            screenVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
            screenVertexArray.Unbind();

            screenShader.Unbind();
            GLCall(glEnable(GL_DEPTH_TEST));

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
