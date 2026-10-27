#pragma once

#include <glad/gles2.h>

#include <cstddef>

// RAII wrapper around an OpenGL array buffer (VBO).
class VertexBuffer
{
public:
    VertexBuffer(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void SetData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);

    void Bind() const;
    void Unbind() const;

    GLuint GetID() const { return m_ID; }
    size_t GetSize() const { return m_Size; }

private:
    GLuint m_ID = 0;
    size_t m_Size = 0;
};

// The task specification refers to the VBO as Buffer as well.
using Buffer = VertexBuffer;

// RAII wrapper around an OpenGL element buffer (EBO).
class IndexBuffer
{
public:
    IndexBuffer(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void SetData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);

    void Bind() const;
    void Unbind() const;

    GLuint GetID() const { return m_ID; }
    size_t GetCount() const { return m_Count; }

private:
    GLuint m_ID = 0;
    size_t m_Size = 0;
    size_t m_Count = 0;
};
