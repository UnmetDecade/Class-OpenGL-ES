#pragma once

#include <glad/gles2.h>

#include <cstddef>
#include <vector>

#include "engine/Renderer/Buffer.h"

struct VertexAttribute
{
    int size;
    GLenum type;
    bool normalized;
    size_t stride;
    size_t offset;
};

// RAII wrapper around an OpenGL vertex array object.
class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    // The vertex buffer must stay alive as long as this VAO is used.
    void AddVertexBuffer(const VertexBuffer& buffer, const VertexAttribute* attributes, size_t count);
    void AddVertexBuffer(const VertexBuffer& buffer, const std::vector<VertexAttribute>& attributes);

    void Bind() const;
    void Unbind() const;

    GLuint GetID() const { return m_ID; }

private:
    GLuint m_ID = 0;
    GLuint m_VertexBufferIndex = 0;
};
