#pragma once

#include "engine/Renderer/RendererAPI.h"

class OpenGLRendererAPI : public RendererAPI
{
public:
    RendererBackend GetBackend() const override { return RendererBackend::OpenGL; }

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
