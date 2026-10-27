#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Cubemap.h"
#include "engine/Renderer/Lighting.h"
#include "engine/Renderer/Mesh.h"
#include "engine/Renderer/Model.h"
#include "engine/Renderer/PostEffect.h"
#include "engine/Renderer/PostProcess.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/ShadowMap.h"
#include "engine/Renderer/Texture.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kGroundY = -0.4f;
const float kGroundExtent = 30.0f;
const float kGroundTiles = 24.0f;

const std::vector<Vertex> kGroundVertices = {
    {{-kGroundExtent, kGroundY, -kGroundExtent}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-kGroundExtent, kGroundY,  kGroundExtent}, {0.0f, 1.0f, 0.0f}, {0.0f, kGroundTiles}},
    {{ kGroundExtent, kGroundY,  kGroundExtent}, {0.0f, 1.0f, 0.0f}, {kGroundTiles, kGroundTiles}},
    {{ kGroundExtent, kGroundY, -kGroundExtent}, {0.0f, 1.0f, 0.0f}, {kGroundTiles, 0.0f}},
};

const std::vector<unsigned int> kGroundIndices = {0, 1, 2, 0, 2, 3};

const float kSkyboxVertices[] = {
    // positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
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
        Window window(1280, 720, "10_skybox - Skybox / Environment Mapping", true, 4);

        const std::string assetDir = ASSET_DIR;

        // --- Shaders ---
        Shader shadowDepthShader(assetDir + "/shaders/shadow_depth.vert",
                                 assetDir + "/shaders/shadow_depth.frag");
        Shader groundShader(assetDir + "/shaders/lighting_shadow.vert",
                            assetDir + "/shaders/lighting_shadow.frag");
        Shader reflectShader(assetDir + "/shaders/lighting_shadow.vert",
                             assetDir + "/shaders/lighting_shadow_reflect.frag");
        Shader skyboxShader(assetDir + "/shaders/skybox.vert",
                            assetDir + "/shaders/skybox.frag");
        // Final output pass: LDR branch with the explicit linear -> sRGB
        // encode (u_EncodeLDR), since the scene targets hold linear values.
        Shader screenShader(assetDir + "/shaders/postprocess.vert",
                            assetDir + "/shaders/hdr_output.frag");

        // --- Scene resources ---
        Model torus(assetDir + "/models/torus.obj");
        Mesh ground(kGroundVertices, kGroundIndices);
        Texture checker(assetDir + "/checker.png");

        const std::array<std::string, 6> skyboxFaces = {
            assetDir + "/textures/skybox/right.png",   // +X
            assetDir + "/textures/skybox/left.png",    // -X
            assetDir + "/textures/skybox/top.png",     // +Y
            assetDir + "/textures/skybox/bottom.png",  // -Y
            assetDir + "/textures/skybox/front.png",   // +Z
            assetDir + "/textures/skybox/back.png",    // -Z
        };
        Cubemap skybox(skyboxFaces);

        // --- Skybox cube ---
        VertexBuffer skyboxVertexBuffer(kSkyboxVertices, sizeof(kSkyboxVertices));
        const VertexAttribute skyboxAttributes[] = {
            {3, GL_FLOAT, false, 3 * sizeof(float), 0},
        };

        VertexArray skyboxVertexArray;
        skyboxVertexArray.Bind();
        skyboxVertexArray.AddVertexBuffer(skyboxVertexBuffer, skyboxAttributes, 1);
        skyboxVertexArray.Unbind();

        // --- Full-screen quad used to present the post-processed texture ---
        VertexBuffer screenVertexBuffer(kScreenQuadVertices, sizeof(kScreenQuadVertices));
        const VertexAttribute screenAttributes[] = {
            {2, GL_FLOAT, false, 4 * sizeof(float), 0},
            {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
        };

        VertexArray screenVertexArray;
        screenVertexArray.Bind();
        screenVertexArray.AddVertexBuffer(screenVertexBuffer, screenAttributes, 2);
        screenVertexArray.Unbind();

        // --- Shadow map + post-process chain ---
        ShadowMap shadowMap(2048);

        PostProcess postProcess(static_cast<unsigned int>(window.GetWidth()),
                                static_cast<unsigned int>(window.GetHeight()));

        const std::string postVertPath = assetDir + "/shaders/postprocess.vert";
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

        // Effects are enabled when added; start with a clean scene so the
        // skybox and reflections are visible immediately.
        selectEffect("");

        // --- Camera and lights ---
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

        GLCall(glEnable(GL_DEPTH_TEST));

        float biasScale = 1.0f;
        int usePCF = 1;
        float reflectivity = 0.5f;
        int useRefraction = 0;

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
            case GLFW_KEY_LEFT_BRACKET:
                biasScale = std::max(0.1f, biasScale - 0.1f);
                LOG_INFO("Shadow bias scale: " << biasScale);
                break;
            case GLFW_KEY_RIGHT_BRACKET:
                biasScale = std::min(5.0f, biasScale + 0.1f);
                LOG_INFO("Shadow bias scale: " << biasScale);
                break;
            case GLFW_KEY_MINUS:
                reflectivity = std::max(0.0f, reflectivity - 0.05f);
                LOG_INFO("Reflectivity: " << reflectivity);
                break;
            case GLFW_KEY_EQUAL:
                reflectivity = std::min(1.0f, reflectivity + 0.05f);
                LOG_INFO("Reflectivity: " << reflectivity);
                break;
            case GLFW_KEY_F6:
                usePCF = usePCF != 0 ? 0 : 1;
                LOG_INFO("PCF 3x3: " << (usePCF != 0 ? "on" : "off"));
                break;
            case GLFW_KEY_F7:
                useRefraction = useRefraction != 0 ? 0 : 1;
                LOG_INFO("Environment mode: " << (useRefraction != 0 ? "refract" : "reflect"));
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

        window.SetFramebufferSizeCallback([&postProcess](int width, int height)
        {
            if (width > 0 && height > 0)
            {
                postProcess.Resize(static_cast<unsigned int>(width),
                                   static_cast<unsigned int>(height));
            }
        });

        LOG_INFO("10_skybox controls: WASD move, Q/E down/up, mouse look, wheel zoom, "
                 "1-6 post effects, [ / ] bias, - / = reflectivity, F6 PCF, F7 refract, ESC quit");

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

            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()));

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

            // --- Scene pass: opaque geometry + skybox into the scene FBO ---
            postProcess.BeginScene();
            Renderer::Clear(0.05f, 0.06f, 0.09f, 1.0f);

            shadowMap.BindForReading(1);
            checker.Bind(0);
            skybox.Bind(2);

            const auto applySceneUniforms = [&](Shader& shader)
            {
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
            };

            applySceneUniforms(groundShader);
            groundShader.SetUniform("uModel", groundModel);
            groundShader.SetUniform("uNormalMatrix", groundNormalMatrix);
            ground.Draw(groundShader);

            applySceneUniforms(reflectShader);
            reflectShader.SetUniform("u_EnvironmentMap", 2);
            reflectShader.SetUniform("u_Reflectivity", reflectivity);
            reflectShader.SetUniform("u_UseRefraction", useRefraction);
            reflectShader.SetUniform("uModel", model);
            reflectShader.SetUniform("uNormalMatrix", normalMatrix);
            torus.Draw(reflectShader);

            skyboxShader.Bind();
            skyboxShader.SetUniform("u_View", view);
            skyboxShader.SetUniform("u_Projection", projection);
            skyboxShader.SetUniform("u_Skybox", 2);

            GLCall(glDepthFunc(GL_LEQUAL));
            skyboxVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));
            skyboxVertexArray.Unbind();
            GLCall(glDepthFunc(GL_LESS));

            skyboxShader.Unbind();
            postProcess.EndScene();

            // --- Post-processing chain + presentation to the default FBO ---
            const unsigned int finalTexture = postProcess.Render();

            GLCall(glViewport(0, 0, window.GetWidth(), window.GetHeight()));
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
