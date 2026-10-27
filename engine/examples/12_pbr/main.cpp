#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
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
#include "engine/Renderer/Framebuffer.h"
#include "engine/Renderer/MaterialPBR.h"
#include "engine/Renderer/Mesh.h"
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

const int kGridColumns = 7;
const int kGridRows = 5;
const float kGridSpacing = 0.8f;
const float kSphereRadius = 0.3f;

const int kPointLightCount = 4;

const std::vector<Vertex> kGroundVertices = {
    {{-kGroundExtent, kGroundY, -kGroundExtent}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-kGroundExtent, kGroundY,  kGroundExtent}, {0.0f, 1.0f, 0.0f}, {0.0f, kGroundTiles}},
    {{ kGroundExtent, kGroundY,  kGroundExtent}, {0.0f, 1.0f, 0.0f}, {kGroundTiles, kGroundTiles}},
    {{ kGroundExtent, kGroundY, -kGroundExtent}, {0.0f, 1.0f, 0.0f}, {kGroundTiles, 0.0f}},
};

const std::vector<unsigned int> kGroundIndices = {0, 1, 2, 0, 2, 3};

const float kSkyboxVertices[] = {
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

const std::array<std::string, kPointLightCount> kPointLightPositionNames = {
    "u_PointLightPositions[0]",
    "u_PointLightPositions[1]",
    "u_PointLightPositions[2]",
    "u_PointLightPositions[3]",
};

const std::array<std::string, kPointLightCount> kPointLightColorNames = {
    "u_PointLightColors[0]",
    "u_PointLightColors[1]",
    "u_PointLightColors[2]",
    "u_PointLightColors[3]",
};

const std::array<std::string, kPointLightCount> kPointLightRadiusNames = {
    "u_PointLightRadius[0]",
    "u_PointLightRadius[1]",
    "u_PointLightRadius[2]",
    "u_PointLightRadius[3]",
};

// UV sphere with smooth (position-derived) normals.
std::vector<Vertex> MakeSphereVertices(unsigned int rings, unsigned int sectors)
{
    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(rings + 1) * (sectors + 1));

    for (unsigned int y = 0; y <= rings; ++y)
    {
        const float v = static_cast<float>(y) / static_cast<float>(rings);
        const float phi = v * glm::pi<float>();

        for (unsigned int x = 0; x <= sectors; ++x)
        {
            const float u = static_cast<float>(x) / static_cast<float>(sectors);
            const float theta = u * glm::two_pi<float>();

            const glm::vec3 normal(std::sin(phi) * std::cos(theta),
                                   std::cos(phi),
                                   std::sin(phi) * std::sin(theta));

            Vertex vertex;
            vertex.position = normal;
            vertex.normal = normal;
            vertex.texCoord = glm::vec2(u, 1.0f - v);
            vertices.push_back(vertex);
        }
    }

    return vertices;
}

std::vector<unsigned int> MakeSphereIndices(unsigned int rings, unsigned int sectors)
{
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<std::size_t>(rings) * sectors * 6);

    for (unsigned int y = 0; y < rings; ++y)
    {
        for (unsigned int x = 0; x < sectors; ++x)
        {
            const unsigned int a = y * (sectors + 1) + x;
            const unsigned int b = a + sectors + 1;

            // CCW when viewed from outside the sphere.
            indices.push_back(a);
            indices.push_back(a + 1);
            indices.push_back(b);

            indices.push_back(a + 1);
            indices.push_back(b + 1);
            indices.push_back(b);
        }
    }

    return indices;
}
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "12_pbr - Metallic-Roughness PBR", true, 4);

        const std::string assetDir = ASSET_DIR;

        // --- Shaders ---
        Shader shadowDepthShader(assetDir + "/shaders/shadow_depth.vert",
                                 assetDir + "/shaders/shadow_depth.frag");
        Shader pbrShader(assetDir + "/shaders/pbr.vert",
                         assetDir + "/shaders/pbr.frag");
        Shader skyboxShader(assetDir + "/shaders/skybox.vert",
                            assetDir + "/shaders/skybox.frag");
        Shader brightShader(assetDir + "/shaders/postprocess.vert",
                            assetDir + "/shaders/hdr_bright.frag");
        Shader blurShader(assetDir + "/shaders/postprocess.vert",
                          assetDir + "/shaders/hdr_blur.frag");

        const auto hdrOutputShader = std::make_shared<Shader>(
            assetDir + "/shaders/postprocess.vert",
            assetDir + "/shaders/hdr_output.frag");

        // --- Geometry ---
        Mesh sphere(MakeSphereVertices(32, 48), MakeSphereIndices(32, 48));
        Mesh ground(kGroundVertices, kGroundIndices);

        // --- Textures / environment ---
        Texture checker(assetDir + "/checker.png");

        const std::array<std::string, 6> skyboxFaces = {
            assetDir + "/textures/skybox/right.png",
            assetDir + "/textures/skybox/left.png",
            assetDir + "/textures/skybox/top.png",
            assetDir + "/textures/skybox/bottom.png",
            assetDir + "/textures/skybox/front.png",
            assetDir + "/textures/skybox/back.png",
        };
        Cubemap skybox(skyboxFaces);

        // Precomputed diffuse irradiance (cosine-weighted convolution of the
        // skybox), loaded with an sRGB internal format like every other image.
        const std::array<std::string, 6> irradianceFaces = {
            assetDir + "/textures/irradiance/right.png",
            assetDir + "/textures/irradiance/left.png",
            assetDir + "/textures/irradiance/top.png",
            assetDir + "/textures/irradiance/bottom.png",
            assetDir + "/textures/irradiance/front.png",
            assetDir + "/textures/irradiance/back.png",
        };
        Cubemap irradianceMap(irradianceFaces);

        // --- Skybox cube ---
        VertexBuffer skyboxVertexBuffer(kSkyboxVertices, sizeof(kSkyboxVertices));
        const VertexAttribute skyboxAttributes[] = {
            {3, GL_FLOAT, false, 3 * sizeof(float), 0},
        };

        VertexArray skyboxVertexArray;
        skyboxVertexArray.Bind();
        skyboxVertexArray.AddVertexBuffer(skyboxVertexBuffer, skyboxAttributes, 1);
        skyboxVertexArray.Unbind();

        // --- Full-screen quad (final output + bloom passes) ---
        VertexBuffer screenVertexBuffer(kScreenQuadVertices, sizeof(kScreenQuadVertices));
        const VertexAttribute screenAttributes[] = {
            {2, GL_FLOAT, false, 4 * sizeof(float), 0},
            {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
        };

        VertexArray screenVertexArray;
        screenVertexArray.Bind();
        screenVertexArray.AddVertexBuffer(screenVertexBuffer, screenAttributes, 2);
        screenVertexArray.Unbind();

        // --- Shadow map, HDR post-process chain, bloom targets ---
        ShadowMap shadowMap(2048);

        bool useHDR = true;
        bool useBloom = true;

        PostProcess postProcess(static_cast<unsigned int>(window.GetWidth()),
                                static_cast<unsigned int>(window.GetHeight()),
                                useHDR);
        postProcess.SetOutputTransform(hdrOutputShader, useHDR);

        const unsigned int halfWidth =
            std::max(1u, static_cast<unsigned int>(window.GetWidth()) / 2u);
        const unsigned int halfHeight =
            std::max(1u, static_cast<unsigned int>(window.GetHeight()) / 2u);

        Framebuffer bloomBrightFramebuffer(halfWidth, halfHeight, true, false);
        Framebuffer bloomFramebufferA(halfWidth, halfHeight, true, false);
        Framebuffer bloomFramebufferB(halfWidth, halfHeight, true, false);

        const std::string postVertPath = assetDir + "/shaders/postprocess.vert";
        const auto makeEffectShader = [&](const char* fragmentFile)
        {
            return std::make_shared<Shader>(postVertPath, assetDir + "/shaders/" + fragmentFile);
        };

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

        selectEffect("");

        // --- Camera ---
        Camera camera(glm::vec3(0.0f, 1.9f, 5.2f), -90.0f, -18.0f);

        // --- Lights ---
        const glm::vec3 dirLightDirection =
            glm::normalize(glm::vec3(-0.55f, -1.0f, -0.35f));
        const glm::vec3 dirLightColor(1.4f, 1.35f, 1.25f);

        // Components above 1.0 survive the RGBA16F scene target (HDR mode).
        const glm::vec3 pointLightColors[kPointLightCount] = {
            glm::vec3(18.0f, 2.2f, 1.8f),   // red
            glm::vec3(2.2f, 16.0f, 3.0f),   // green
            glm::vec3(1.8f, 3.2f, 20.0f),   // blue
            glm::vec3(14.0f, 13.0f, 12.0f), // warm white
        };
        const float pointLightRadii[kPointLightCount] = {7.0f, 7.5f, 8.0f, 7.0f};

        const glm::vec3 lightDir = dirLightDirection;
        const glm::vec3 lightPosition = -lightDir * 10.0f;
        const glm::mat4 lightView = glm::lookAt(lightPosition,
                                                glm::vec3(0.0f),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f,
                                                     -10.0f, 10.0f,
                                                     1.0f, 20.0f);
        const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // --- Static material/transform setup for the 7x5 sphere grid ---
        struct SphereInstance
        {
            glm::mat4 Model{1.0f};
            glm::mat3 NormalMatrix{1.0f};
            float Metallic = 0.0f;
            float Roughness = 0.5f;
        };

        const int sphereCount = kGridColumns * kGridRows;
        std::vector<SphereInstance> spheres(static_cast<std::size_t>(sphereCount));

        for (int row = 0; row < kGridRows; ++row)
        {
            for (int column = 0; column < kGridColumns; ++column)
            {
                SphereInstance& instance =
                    spheres[static_cast<std::size_t>(row * kGridColumns + column)];

                const float metallic =
                    static_cast<float>(column) / static_cast<float>(kGridColumns - 1);
                // Smooth row nearest the camera (row 0), rough row furthest away.
                const float roughness =
                    0.05f + 0.95f * static_cast<float>(row) /
                                     static_cast<float>(kGridRows - 1);

                const glm::vec3 position(
                    (static_cast<float>(column) - (kGridColumns - 1) * 0.5f) * kGridSpacing,
                    kGroundY + kSphereRadius,
                    (static_cast<float>(kGridRows - 1) * 0.5f - static_cast<float>(row)) *
                        kGridSpacing);

                instance.Model = glm::translate(glm::mat4(1.0f), position) *
                                 glm::scale(glm::mat4(1.0f),
                                            glm::vec3(kSphereRadius));
                instance.NormalMatrix =
                    glm::mat3(glm::transpose(glm::inverse(instance.Model)));
                instance.Metallic = metallic;
                instance.Roughness = roughness;
            }
        }

        MaterialPBR sphereMaterial;
        sphereMaterial.Albedo = glm::vec3(0.955f, 0.638f, 0.538f);
        sphereMaterial.Metallic = 0.0f;
        sphereMaterial.Roughness = 0.5f;
        sphereMaterial.AO = 1.0f;

        MaterialPBR groundMaterial;
        groundMaterial.Albedo = glm::vec3(0.9f);
        groundMaterial.Metallic = 0.0f;
        groundMaterial.Roughness = 0.9f;
        groundMaterial.AO = 1.0f;

        const glm::mat4 groundModel = glm::mat4(1.0f);
        const glm::mat3 groundNormalMatrix = glm::mat3(1.0f);

        GLCall(glEnable(GL_DEPTH_TEST));

        float biasScale = 1.0f;
        int usePCF = 1;
        int useIBL = 1;
        int lambertOnly = 0;
        bool lightsRotating = true;
        float lightTime = 0.0f;

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
            case GLFW_KEY_F6:
                lightsRotating = !lightsRotating;
                LOG_INFO("Point light rotation: "
                         << (lightsRotating ? "running" : "paused"));
                break;
            case GLFW_KEY_F7:
                useIBL = useIBL != 0 ? 0 : 1;
                LOG_INFO("Diffuse IBL: " << (useIBL != 0 ? "on" : "off"));
                break;
            case GLFW_KEY_F8:
                useHDR = !useHDR;
                postProcess.SetHDRScene(useHDR);
                postProcess.SetOutputTransform(hdrOutputShader, useHDR);
                LOG_INFO("Render mode: "
                         << (useHDR ? "HDR (RGBA16F, Reinhard + gamma)"
                                    : "LDR (RGB8, lights clamped to 1.0)"));
                break;
            case GLFW_KEY_F9:
                useBloom = !useBloom;
                LOG_INFO("Bloom: " << (useBloom ? "on" : "off")
                                   << " (only visible in HDR mode)");
                break;
            case GLFW_KEY_L:
                lambertOnly = lambertOnly != 0 ? 0 : 1;
                LOG_INFO("BRDF mode: " << (lambertOnly != 0
                                               ? "Lambert diffuse only (kd)"
                                               : "full Cook-Torrance"));
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

        window.SetFramebufferSizeCallback(
            [&postProcess, &bloomBrightFramebuffer, &bloomFramebufferA, &bloomFramebufferB]
            (int width, int height)
            {
                if (width <= 0 || height <= 0)
                {
                    return;
                }

                postProcess.Resize(static_cast<unsigned int>(width),
                                   static_cast<unsigned int>(height));

                const unsigned int bloomWidth =
                    std::max(1u, static_cast<unsigned int>(width) / 2u);
                const unsigned int bloomHeight =
                    std::max(1u, static_cast<unsigned int>(height) / 2u);
                bloomBrightFramebuffer.Resize(bloomWidth, bloomHeight);
                bloomFramebufferA.Resize(bloomWidth, bloomHeight);
                bloomFramebufferB.Resize(bloomWidth, bloomHeight);
            });

        LOG_INFO("12_pbr controls: WASD move, Q/E down/up, mouse look, wheel zoom, "
                 "1-6 post effects, [ / ] shadow bias, L Lambert-only, "
                 "F6 light rotation, F7 diffuse IBL, F8 HDR/LDR, F9 bloom, ESC quit");

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

            if (lightsRotating)
            {
                lightTime += deltaTime;
            }

            std::array<glm::vec3, kPointLightCount> pointLightPositions;
            for (int i = 0; i < kPointLightCount; ++i)
            {
                const float phase =
                    lightTime * 0.7f + static_cast<float>(i) * glm::half_pi<float>();
                pointLightPositions[static_cast<std::size_t>(i)] =
                    glm::vec3(std::cos(phase) * 2.4f,
                              1.5f + std::sin(lightTime * 0.5f + static_cast<float>(i)) * 0.35f,
                              std::sin(phase) * 2.4f);
            }

            std::array<glm::vec3, kPointLightCount> effectivePointLightColors;
            for (int i = 0; i < kPointLightCount; ++i)
            {
                effectivePointLightColors[static_cast<std::size_t>(i)] =
                    useHDR ? pointLightColors[i]
                           : glm::min(pointLightColors[i], glm::vec3(1.0f));
            }
            const glm::vec3 effectiveDirLightColor =
                useHDR ? dirLightColor : glm::min(dirLightColor, glm::vec3(1.0f));

            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()));

            // --- Shadow pass (directional light only, same path as phase 9) ---
            shadowMap.BeginWrite(lightSpaceMatrix);

            shadowDepthShader.Bind();
            shadowDepthShader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
            shadowDepthShader.SetUniform("u_Model", groundModel);
            ground.Draw(shadowDepthShader);

            for (const SphereInstance& instance : spheres)
            {
                shadowDepthShader.Bind();
                shadowDepthShader.SetUniform("u_Model", instance.Model);
                sphere.Draw(shadowDepthShader);
            }

            shadowMap.EndWrite(static_cast<unsigned int>(window.GetWidth()),
                               static_cast<unsigned int>(window.GetHeight()));

            // --- Scene pass into the (HDR or LDR) scene framebuffer ---
            postProcess.BeginScene();
            Renderer::Clear(0.05f, 0.06f, 0.09f, 1.0f);

            shadowMap.BindForReading(1);
            checker.Bind(0);
            skybox.Bind(2);
            irradianceMap.Bind(3);

            pbrShader.Bind();
            pbrShader.SetUniform("uView", view);
            pbrShader.SetUniform("uProjection", projection);
            pbrShader.SetUniform("uViewPos", camera.GetPosition());
            pbrShader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
            pbrShader.SetUniform("u_BiasScale", biasScale);
            pbrShader.SetUniform("u_UsePCF", usePCF);
            pbrShader.SetUniform("u_AmbientColor", glm::vec3(0.03f));
            pbrShader.SetUniform("u_LambertOnly", lambertOnly);
            pbrShader.SetUniform("u_UseIBL", useIBL);
            pbrShader.SetUniform("u_AlbedoMap", 0);
            pbrShader.SetUniform("u_IrradianceMap", 3);
            pbrShader.SetUniform("u_ShadowMap", 1);
            pbrShader.SetUniform("u_NumPointLights", kPointLightCount);
            pbrShader.SetUniform("u_DirLightDirection", dirLightDirection);
            pbrShader.SetUniform("u_DirLightColor", effectiveDirLightColor);

            for (int i = 0; i < kPointLightCount; ++i)
            {
                const std::size_t index = static_cast<std::size_t>(i);
                pbrShader.SetUniform(kPointLightPositionNames[index],
                                     pointLightPositions[index]);
                pbrShader.SetUniform(kPointLightColorNames[index],
                                     effectivePointLightColors[index]);
                pbrShader.SetUniform(kPointLightRadiusNames[index],
                                     pointLightRadii[index]);
            }

            // Checker ground: albedo comes from the texture.
            pbrShader.SetUniform("u_UseAlbedoMap", 1);
            pbrShader.SetUniform("uModel", groundModel);
            pbrShader.SetUniform("uNormalMatrix", groundNormalMatrix);
            groundMaterial.Apply(pbrShader);
            ground.Draw(pbrShader);

            // Sphere grid: column = metallic, row = roughness.
            // Mesh::Draw unbinds the program, so re-bind before touching
            // another uniform.
            pbrShader.Bind();
            pbrShader.SetUniform("u_UseAlbedoMap", 0);
            for (const SphereInstance& instance : spheres)
            {
                pbrShader.Bind();
                pbrShader.SetUniform("uModel", instance.Model);
                pbrShader.SetUniform("uNormalMatrix", instance.NormalMatrix);

                sphereMaterial.Metallic = instance.Metallic;
                sphereMaterial.Roughness = instance.Roughness;
                sphereMaterial.Apply(pbrShader);

                sphere.Draw(pbrShader);
            }

            // --- Skybox background ---
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

            // --- Post-process chain ---
            const unsigned int finalTexture = postProcess.Render();

            const bool applyBloom = useHDR && useBloom;
            unsigned int bloomTexture = 0;

            if (applyBloom)
            {
                const unsigned int bloomWidth = bloomBrightFramebuffer.GetWidth();
                const unsigned int bloomHeight = bloomBrightFramebuffer.GetHeight();

                // Bright pass.
                bloomBrightFramebuffer.Bind();
                GLCall(glViewport(0, 0,
                                  static_cast<GLsizei>(bloomWidth),
                                  static_cast<GLsizei>(bloomHeight)));
                brightShader.Bind();
                brightShader.SetUniform("u_ScreenTexture", 0);
                brightShader.SetUniform("u_Threshold", 1.0f);

                GLCall(glActiveTexture(GL_TEXTURE0));
                GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));

                GLCall(glDisable(GL_DEPTH_TEST));
                screenVertexArray.Bind();
                GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
                screenVertexArray.Unbind();

                brightShader.Unbind();

                // Horizontal blur.
                bloomFramebufferA.Bind();
                GLCall(glViewport(0, 0,
                                  static_cast<GLsizei>(bloomWidth),
                                  static_cast<GLsizei>(bloomHeight)));
                blurShader.Bind();
                blurShader.SetUniform("u_ScreenTexture", 0);
                blurShader.SetUniform("u_Horizontal", 1);

                GLCall(glActiveTexture(GL_TEXTURE0));
                GLCall(glBindTexture(GL_TEXTURE_2D,
                                     bloomBrightFramebuffer.GetColorAttachmentID()));

                screenVertexArray.Bind();
                GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
                screenVertexArray.Unbind();
                blurShader.Unbind();

                // Vertical blur.
                bloomFramebufferB.Bind();
                GLCall(glViewport(0, 0,
                                  static_cast<GLsizei>(bloomWidth),
                                  static_cast<GLsizei>(bloomHeight)));
                blurShader.Bind();
                blurShader.SetUniform("u_ScreenTexture", 0);
                blurShader.SetUniform("u_Horizontal", 0);

                GLCall(glActiveTexture(GL_TEXTURE0));
                GLCall(glBindTexture(GL_TEXTURE_2D,
                                     bloomFramebufferA.GetColorAttachmentID()));

                screenVertexArray.Bind();
                GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
                screenVertexArray.Unbind();
                blurShader.Unbind();

                bloomTexture = bloomFramebufferB.GetColorAttachmentID();
            }

            // --- Final output pass: tonemap + gamma (HDR) or pass-through (LDR) ---
            Framebuffer::Unbind();
            GLCall(glViewport(0, 0, window.GetWidth(), window.GetHeight()));
            GLCall(glDisable(GL_DEPTH_TEST));

            hdrOutputShader->Bind();
            hdrOutputShader->SetUniform("u_ScreenTexture", 0);
            hdrOutputShader->SetUniform("u_BloomTexture", 1);
            hdrOutputShader->SetUniform("u_UseBloom", applyBloom ? 1 : 0);
            hdrOutputShader->SetUniform("u_BloomStrength", 0.8f);

            // When bloom is off the shader's bloom branch is not taken, but
            // the sampler still needs a complete texture bound to its unit
            // (an unbound sampler emits a Metal "unloadable texture" warning).
            const unsigned int bloomSampleTexture =
                applyBloom ? bloomTexture
                           : bloomBrightFramebuffer.GetColorAttachmentID();

            GLCall(glActiveTexture(GL_TEXTURE0));
            GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
            GLCall(glActiveTexture(GL_TEXTURE1));
            GLCall(glBindTexture(GL_TEXTURE_2D, bloomSampleTexture));

            screenVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
            screenVertexArray.Unbind();

            hdrOutputShader->Unbind();

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
