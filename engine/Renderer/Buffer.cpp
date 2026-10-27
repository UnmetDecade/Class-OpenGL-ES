#include "engine/Renderer/Buffer.h"

#include <utility>

#include "engine/Core/GLCall.h"

VertexBuffer::VertexBuffer(const void* data, size_t size, GLenum usage)
    : m_Size(size)
{
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

VertexBuffer::~VertexBuffer()
{
    if (m_ID != 0)
    {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
    }
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_ID(other.m_ID), m_Size(other.m_Size)
{
    other.m_ID = 0;
    other.m_Size = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_ID != 0)
        {
            GLCall(glDeleteBuffers(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_Size = other.m_Size;

        other.m_ID = 0;
        other.m_Size = 0;
    }
    return *this;
}

void VertexBuffer::SetData(const void* data, size_t size, GLenum usage)
{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    m_Size = size;
}

void VertexBuffer::Bind() const
{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
}

void VertexBuffer::Unbind() const
{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

IndexBuffer::IndexBuffer(const void* data, size_t size, GLenum usage)
    : m_Size(size), m_Count(size / sizeof(GLuint))
{
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

IndexBuffer::~IndexBuffer()
{
    if (m_ID != 0)
    {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
    }
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_ID(other.m_ID), m_Size(other.m_Size), m_Count(other.m_Count)
{
    other.m_ID = 0;
    other.m_Size = 0;
    other.m_Count = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_ID != 0)
        {
            GLCall(glDeleteBuffers(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_Size = other.m_Size;
        m_Count = other.m_Count;

        other.m_ID = 0;
        other.m_Size = 0;
        other.m_Count = 0;
    }
    return *this;
}

void IndexBuffer::SetData(const void* data, size_t size, GLenum usage)
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, usage));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    m_Size = size;
    m_Count = size / sizeof(GLuint);
}

void IndexBuffer::Bind() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID));
}

void IndexBuffer::Unbind() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
