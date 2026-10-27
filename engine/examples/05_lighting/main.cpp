#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <exception>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Lighting.h"
#include "engine/Renderer/Model.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/Texture.h"
#include "engine/Window/Window.h"

int main()
{
    try
    {
        Window window(1280, 720, "05_lighting - Material / Model / MSAA", true, 4);

        const std::string assetDir = ASSET_DIR;

        Shader shader(assetDir + "/shaders/lighting.vert",
                      assetDir + "/shaders/lighting.frag");
        Model torus(assetDir + "/models/torus.obj");
        Texture checker(assetDir + "/checker.png");

        GLCall(glEnable(GL_DEPTH_TEST));

        Camera camera(glm::vec3(0.0f, 0.8f, 3.6f), -90.0f, -8.0f);

        Material material;
        material.ambient = glm::vec3(0.35f);
        material.diffuse = glm::vec3(0.9f);
        material.specular = glm::vec3(0.6f);
        material.shininess = 64.0f;

        DirectionalLight dirLight;
        dirLight.direction = glm::vec3(-0.4f, -1.0f, -0.3f);
        dirLight.ambient = glm::vec3(0.15f);
        dirLight.diffuse = glm::vec3(0.75f, 0.8f, 0.9f);
        dirLight.specular = glm::vec3(1.0f);

        PointLight pointLight;
        pointLight.constant = 1.0f;
        pointLight.linear = 0.35f;
        pointLight.quadratic = 0.44f;
        pointLight.diffuse = glm::vec3(1.0f, 0.55f, 0.25f);
        pointLight.specular = glm::vec3(1.0f);

        GLFWwindow* nativeWindow = window.GetNativeHandle();
        glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        window.SetKeyCallback([nativeWindow](int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(nativeWindow, GLFW_TRUE);
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

        LOG_INFO("05_lighting controls: WASD move, Q/E down/up, mouse look, wheel zoom, ESC quit");

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

            Renderer::Clear(0.05f, 0.06f, 0.09f, 1.0f);

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                                currentFrame * 0.35f,
                                                glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()));

            shader.Bind();
            shader.SetUniform("uModel", model);
            shader.SetUniform("uView", view);
            shader.SetUniform("uProjection", projection);
            shader.SetUniform("uNormalMatrix",
                              glm::mat3(glm::transpose(glm::inverse(model))));
            shader.SetUniform("uViewPos", camera.GetPosition());
            shader.SetUniform("uTexture", 0);

            material.Apply(shader);
            dirLight.Apply(shader);
            pointLight.Apply(shader);

            checker.Bind(0);
            torus.Draw(shader);

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
