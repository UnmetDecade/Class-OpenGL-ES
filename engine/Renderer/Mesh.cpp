#include "engine/Renderer/Mesh.h"

#include <cstddef>
#include <utility>

#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    : m_VertexBuffer(vertices.data(), vertices.size() * sizeof(Vertex)),
      m_IndexBuffer(indices.data(), indices.size() * sizeof(unsigned int)),
      m_VertexArray(),
      m_IndexCount(indices.size())
{
    const VertexAttribute attributes[] = {
        {3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, position)},
        {3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, normal)},
        {2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, texCoord)},
    };

    m_VertexArray.Bind();
    m_VertexArray.AddVertexBuffer(m_VertexBuffer, attributes, 3);

    m_VertexArray.Bind();
    m_IndexBuffer.Bind();
    m_VertexArray.Unbind();
}

void Mesh::Draw(const Shader& shader) const
{
    Renderer::Draw(m_VertexArray, m_IndexBuffer, shader);
}
