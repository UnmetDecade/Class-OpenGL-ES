#include "engine/Renderer/VertexArray.h"

#include <utility>

#include "engine/Core/GLCall.h"

VertexArray::VertexArray()
{
    GLCall(glGenVertexArrays(1, &m_ID));
}

VertexArray::~VertexArray()
{
    if (m_ID != 0)
    {
        GLCall(glDeleteVertexArrays(1, &m_ID));
        m_ID = 0;
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_ID(other.m_ID), m_VertexBufferIndex(other.m_VertexBufferIndex)
{
    other.m_ID = 0;
    other.m_VertexBufferIndex = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other)
    {
        if (m_ID != 0)
        {
            GLCall(glDeleteVertexArrays(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_VertexBufferIndex = other.m_VertexBufferIndex;

        other.m_ID = 0;
        other.m_VertexBufferIndex = 0;
    }
    return *this;
}

void VertexArray::AddVertexBuffer(const VertexBuffer& buffer, const VertexAttribute* attributes, size_t count)
{
    GLCall(glBindVertexArray(m_ID));
    buffer.Bind();

    for (size_t i = 0; i < count; ++i)
    {
        const VertexAttribute& attribute = attributes[i];

        GLCall(glVertexAttribPointer(m_VertexBufferIndex,
                                     attribute.size,
                                     attribute.type,
                                     attribute.normalized ? GL_TRUE : GL_FALSE,
                                     static_cast<GLsizei>(attribute.stride),
                                     reinterpret_cast<const void*>(attribute.offset)));
        GLCall(glEnableVertexAttribArray(m_VertexBufferIndex));

        ++m_VertexBufferIndex;
    }

    buffer.Unbind();
    GLCall(glBindVertexArray(0));
}

void VertexArray::AddVertexBuffer(const VertexBuffer& buffer, const std::vector<VertexAttribute>& attributes)
{
    AddVertexBuffer(buffer, attributes.data(), attributes.size());
}

void VertexArray::Bind() const
{
    GLCall(glBindVertexArray(m_ID));
}

void VertexArray::Unbind() const
{
    GLCall(glBindVertexArray(0));
}
