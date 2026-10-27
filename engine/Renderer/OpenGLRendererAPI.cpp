#include "engine/Renderer/OpenGLRendererAPI.h"

#include <glad/gles2.h>

#include <stdexcept>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/VertexArray.h"

namespace
{
GLenum ToOpenGLDrawMode(DrawMode mode)
{
    switch (mode)
    {
    case DrawMode::Triangles: return GL_TRIANGLES;
    case DrawMode::Lines:     return GL_LINES;
    case DrawMode::Points:    return GL_POINTS;
    }
    throw std::runtime_error("Unsupported DrawMode");
}
} // namespace

void OpenGLRendererAPI::Init()
{
    LOG_INFO("OpenGL renderer backend initialized");
}

void OpenGLRendererAPI::SetClearColor(float r, float g, float b, float a)
{
    GLCall(glClearColor(r, g, b, a));
}

void OpenGLRendererAPI::Clear()
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    ++m_Stats.ClearCalls;
}

void OpenGLRendererAPI::DrawIndexed(const VertexArray& vertexArray,
                                    const IndexBuffer& indexBuffer,
                                    const Shader& shader)
{
    shader.Bind();
    vertexArray.Bind();
    indexBuffer.Bind();

    GLCall(glDrawElements(GL_TRIANGLES,
                          static_cast<GLsizei>(indexBuffer.GetCount()),
                          GL_UNSIGNED_INT,
                          nullptr));

    // Do not unbind the element buffer here: the VAO remembers it and
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) would clear that bindng.
    vertexArray.Unbind();
    shader.Unbind();

    ++m_Stats.DrawCalls;
    m_Stats.IndexCount += indexBuffer.GetCount();
}

void OpenGLRendererAPI::DrawArrays(const VertexArray& vertexArray,
                                   const Shader& shader,
                                   DrawMode mode,
                                   int count,
                                   int first)
{
    shader.Bind();
    vertexArray.Bind();

    GLCall(glDrawArrays(ToOpenGLDrawMode(mode), first, count));

    vertexArray.Unbind();
    shader.Unbind();

    ++m_Stats.DrawArraysCalls;
    m_Stats.VertexCount += static_cast<std::uint64_t>(count);
}

void OpenGLRendererAPI::SetDepthTest(bool enabled)
{
    if (enabled)
    {
        GLCall(glEnable(GL_DEPTH_TEST));
    }
    else
    {
        GLCall(glDisable(GL_DEPTH_TEST));
    }
}

void OpenGLRendererAPI::SetViewport(int x, int y, int width, int height)
{
    GLCall(glViewport(x, y, width, height));
}
