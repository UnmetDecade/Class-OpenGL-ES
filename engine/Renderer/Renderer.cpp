#include "engine/Renderer/Renderer.h"

#include <memory>
#include <stdexcept>

#include "engine/Core/Log.h"
#include "engine/Renderer/NullRendererAPI.h"
#include "engine/Renderer/OpenGLRendererAPI.h"

namespace
{
std::unique_ptr<RendererAPI> g_API;

const char* BackendName(RendererBackend backend)
{
    switch (backend)
    {
    case RendererBackend::OpenGL: return "OpenGL";
    case RendererBackend::Null:   return "Null";
    }
    return "Unknown";
}

void EnsureAPI()
{
    if (!g_API)
    {
        g_API = std::make_unique<OpenGLRendererAPI>();
        g_API->Init();
    }
}

DrawMode ToDrawMode(GLenum mode)
{
    switch (mode)
    {
    case GL_TRIANGLES: return DrawMode::Triangles;
    case GL_LINES:     return DrawMode::Lines;
    case GL_POINTS:    return DrawMode::Points;
    default:           throw std::runtime_error("Renderer: unsupported draw mode");
    }
}
} // namespace

void Renderer::SetBackend(RendererBackend backend)
{
    if (g_API && g_API->GetBackend() == backend)
    {
        return;
    }

    if (g_API)
    {
        const RendererStats& stats = g_API->GetStats();
        LOG_INFO("Renderer leaving backend " << BackendName(g_API->GetBackend())
                 << " | clear=" << stats.ClearCalls
                 << " draws=" << stats.DrawCalls
                 << " drawArrays=" << stats.DrawArraysCalls
                 << " indices=" << stats.IndexCount
                 << " vertices=" << stats.VertexCount);
    }

    if (backend == RendererBackend::OpenGL)
    {
        g_API = std::make_unique<OpenGLRendererAPI>();
    }
    else
    {
        g_API = std::make_unique<NullRendererAPI>();
    }

    g_API->Init();
    LOG_INFO("Renderer backend switched to " << BackendName(backend));
}

RendererBackend Renderer::GetBackend()
{
    EnsureAPI();
    return g_API->GetBackend();
}

const RendererStats& Renderer::GetStats()
{
    EnsureAPI();
    return g_API->GetStats();
}

void Renderer::Clear(float r, float g, float b, float a)
{
    EnsureAPI();
    g_API->SetClearColor(r, g, b, a);
    g_API->Clear();
}

void Renderer::Draw(const VertexArray& vertexArray,
                    const IndexBuffer& indexBuffer,
                    const Shader& shader)
{
    EnsureAPI();
    g_API->DrawIndexed(vertexArray, indexBuffer, shader);
}

void Renderer::DrawArrays(const VertexArray& vertexArray,
                          const Shader& shader,
                          GLenum mode,
                          GLsizei count,
                          GLint first)
{
    EnsureAPI();
    g_API->DrawArrays(vertexArray, shader, ToDrawMode(mode), count, first);
}
