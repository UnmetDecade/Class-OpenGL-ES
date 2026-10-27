#pragma once

#include <glad/gles2.h>

#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/RendererAPI.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/VertexArray.h"

// Stateless drawing helpers. All commands are forwarded to the active
// RendererAPI backend, which can be switched at runtime.
class Renderer
{
public:
    Renderer() = delete;

    static void SetBackend(RendererBackend backend);
    static RendererBackend GetBackend();
    static const RendererStats& GetStats();

    static void Clear(float r, float g, float b, float a);

    static void Draw(const VertexArray& vertexArray,
                     const IndexBuffer& indexBuffer,
                     const Shader& shader);

    static void DrawArrays(const VertexArray& vertexArray,
                           const Shader& shader,
                           GLenum mode,
                           GLsizei count,
                           GLint first = 0);
};
