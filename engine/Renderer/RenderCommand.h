#pragma once

#include <glad/gles2.h>

#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/VertexArray.h"

// Thin command layer in front of Renderer. Keeping call sites on this
// interface leaves room for a different backend later.
class RenderCommand
{
public:
    RenderCommand() = delete;

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
