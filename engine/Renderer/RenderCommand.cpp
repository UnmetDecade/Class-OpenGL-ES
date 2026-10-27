#include "engine/Renderer/RenderCommand.h"

#include "engine/Renderer/Renderer.h"

void RenderCommand::Clear(float r, float g, float b, float a)
{
    Renderer::Clear(r, g, b, a);
}

void RenderCommand::Draw(const VertexArray& vertexArray,
                         const IndexBuffer& indexBuffer,
                         const Shader& shader)
{
    Renderer::Draw(vertexArray, indexBuffer, shader);
}

void RenderCommand::DrawArrays(const VertexArray& vertexArray,
                               const Shader& shader,
                               GLenum mode,
                               GLsizei count,
                               GLint first)
{
    Renderer::DrawArrays(vertexArray, shader, mode, count, first);
}
