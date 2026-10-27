#pragma once

#include <cstdint>

class VertexArray;
class IndexBuffer;
class Shader;

enum class RendererBackend
{
    OpenGL,
    Null,
};

enum class DrawMode
{
    Triangles,
    Lines,
    Points,
};

struct RendererStats
{
    std::uint64_t ClearCalls = 0;
    std::uint64_t DrawCalls = 0;
    std::uint64_t DrawArraysCalls = 0;
    std::uint64_t IndexCount = 0;
    std::uint64_t VertexCount = 0;
};

// Command-level backend interface. Renderer owns the active implementation and
// forwards all drawing through it, so backends can be swapped at runtime.
class RendererAPI
{
public:
    virtual ~RendererAPI() = default;

    virtual RendererBackend GetBackend() const = 0;

    virtual void Init() = 0;
    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(const VertexArray& vertexArray,
                             const IndexBuffer& indexBuffer,
                             const Shader& shader) = 0;

    virtual void DrawArrays(const VertexArray& vertexArray,
                            const Shader& shader,
                            DrawMode mode,
                            int count,
                            int first) = 0;

    virtual void SetDepthTest(bool enabled) = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;

    const RendererStats& GetStats() const { return m_Stats; }

protected:
    RendererStats m_Stats;
};
