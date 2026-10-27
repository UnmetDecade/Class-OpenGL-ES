#include <glad/gles2.h>

#include <glm/glm.hpp>

#include <exception>
#include <memory>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/RenderCommand.h"
#include "engine/Renderer/ShaderLibrary.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
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
        Window window(1280, 720, "04_shader_library - Stage 4");

        const std::string assetDir = ASSET_DIR;

        ShaderLibrary shaderLibrary;
        std::shared_ptr<Shader> shader = shaderLibrary.Load(assetDir + "/shaders/basic.glsl");

        VertexBuffer vertexBuffer(kVertices, sizeof(kVertices));

        const VertexAttribute attributes[] = {
            {3, GL_FLOAT, false, 3 * sizeof(float), 0},
        };

        VertexArray vertexArray;
        vertexArray.Bind();
        vertexArray.AddVertexBuffer(vertexBuffer, attributes, 1);
        vertexArray.Unbind();

        while (!window.ShouldClose())
        {
            window.PollEvents();

            RenderCommand::Clear(0.1f, 0.15f, 0.25f, 1.0f);

            shader->Bind();
            shader->SetUniform("uColor", glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
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
