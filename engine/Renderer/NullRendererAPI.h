#pragma once

#include "engine/Renderer/RendererAPI.h"

// Backend used for headless runs and backend-switch tests. It never touches
// OpenGL; it only validates the command stream and records statistics.
class NullRendererAPI : public RendererAPI
{
public:
    RendererBackend GetBackend() const override { return RendererBackend::Null; }

    void Init() override;
    void SetClearColor(float r, float g, float b, float a) override;
    void Clear() override;

    void DrawIndexed(const VertexArray& vertexArray,
                     const IndexBuffer& indexBuffer,
                     const Shader& shader) override;

    void DrawArrays(const VertexArray& vertexArray,
                    const Shader& shader,
                    DrawMode mode,
                    int count,
                    int first) override;

    void SetDepthTest(bool enabled) override;
    void SetViewport(int x, int y, int width, int height) override;
};
