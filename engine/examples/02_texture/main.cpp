#include <glad/gles2.h>

#include <exception>
#include <string>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/Texture.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

namespace
{
const float kVertices[] = {
    // positions         // tex coords
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
};

const unsigned int kIndices[] = {
    0, 1, 2,
    2, 3, 0,
};
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "02_texture - Stage 2");

        const std::string assetDir = ASSET_DIR;
        Shader shader(assetDir + "/shaders/texture.vert",
                      assetDir + "/shaders/texture.frag");

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

        Texture texture(assetDir + "/container.jpg");

        while (!window.ShouldClose())
        {
            window.PollEvents();

            GLCall(glClearColor(0.1f, 0.15f, 0.25f, 1.0f));
            GLCall(glClear(GL_COLOR_BUFFER_BIT));

            shader.Bind();
            shader.SetUniform("uTexture", 0);

            texture.Bind(0);
            vertexArray.Bind();

            GLCall(glDrawElements(GL_TRIANGLES,
                                  static_cast<GLsizei>(indexBuffer.GetCount()),
                                  GL_UNSIGNED_INT,
                                  nullptr));

            shader.Unbind();
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
