#include "engine/Renderer/NullRendererAPI.h"

#include "engine/Core/Log.h"
#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/VertexArray.h"

void NullRendererAPI::Init()
{
    LOG_INFO("Null renderer backend initialized (no GPU commands will be issued)");
}

void NullRendererAPI::SetClearColor(float r, float g, float b, float a)
{
    (void)r;
    (void)g;
    (void)b;
    (void)a;
}

void NullRendererAPI::Clear()
{
    ++m_Stats.ClearCalls;
}

void NullRendererAPI::DrawIndexed(const VertexArray& vertexArray,
                                  const IndexBuffer& indexBuffer,
                                  const Shader& shader)
{
    (void)vertexArray;
    (void)shader;

    if (indexBuffer.GetID() == 0)
    {
        LOG_WARN("NullRendererAPI: draw called with an invalid index buffer");
        return;
    }

    ++m_Stats.DrawCalls;
    m_Stats.IndexCount += indexBuffer.GetCount();
}

void NullRendererAPI::DrawArrays(const VertexArray& vertexArray,
                                 const Shader& shader,
                                 DrawMode mode,
                                 int count,
                                 int first)
{
    (void)shader;
    (void)mode;
    (void)first;

    if (vertexArray.GetID() == 0)
    {
        LOG_WARN("NullRendererAPI: draw called with an invalid vertex array");
        return;
    }

    ++m_Stats.DrawArraysCalls;
    m_Stats.VertexCount += static_cast<std::uint64_t>(count);
}

void NullRendererAPI::SetDepthTest(bool enabled)
{
    (void)enabled;
}

void NullRendererAPI::SetViewport(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
