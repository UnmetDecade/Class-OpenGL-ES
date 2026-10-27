#pragma once

#include <glad/gles2.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/VertexArray.h"

class Shader;

struct Vertex
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 0.0f};
    glm::vec2 texCoord{0.0f, 0.0f};
};

// Owns one VAO/VBO/EBO triple for a triangle mesh.
class Mesh
{
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void Draw(const Shader& shader) const;

    size_t GetIndexCount() const { return m_IndexCount; }

private:
    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    VertexArray m_VertexArray;
    size_t m_IndexCount = 0;
};
