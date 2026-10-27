#include "engine/Renderer/Model.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "engine/Core/Log.h"
#include "engine/Renderer/Shader.h"

namespace
{
struct ObjCorner
{
    int position = -1;
    int texCoord = -1;
    int normal = -1;
};

struct ObjTriangle
{
    ObjCorner corners[3];
};

int ParseObjIndex(const std::string& token, size_t count, size_t lineNumber)
{
    if (token.empty())
    {
        return -1;
    }

    int raw = 0;
    try
    {
        raw = std::stoi(token);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("OBJ: invalid index '" + token +
                                 "' at line " + std::to_string(lineNumber));
    }

    if (raw < 0)
    {
        raw = static_cast<int>(count) + raw;
    }
    else
    {
        raw -= 1;
    }

    if (raw < 0 || static_cast<size_t>(raw) >= count)
    {
        throw std::runtime_error("OBJ: index out of range at line " +
                                 std::to_string(lineNumber));
    }

    return raw;
}

ObjCorner ParseCorner(const std::string& token,
                      size_t positionCount,
                      size_t texCoordCount,
                      size_t normalCount,
                      size_t lineNumber)
{
    ObjCorner corner;

    const size_t firstSlash = token.find('/');
    corner.position = ParseObjIndex(token.substr(0, firstSlash), positionCount, lineNumber);

    if (firstSlash != std::string::npos)
    {
        const size_t secondSlash = token.find('/', firstSlash + 1);

        const std::string texCoordToken =
            (secondSlash == std::string::npos)
                ? token.substr(firstSlash + 1)
                : token.substr(firstSlash + 1, secondSlash - firstSlash - 1);

        if (!texCoordToken.empty())
        {
            corner.texCoord = ParseObjIndex(texCoordToken, texCoordCount, lineNumber);
        }

        if (secondSlash != std::string::npos)
        {
            const std::string normalToken = token.substr(secondSlash + 1);
            if (!normalToken.empty())
            {
                corner.normal = ParseObjIndex(normalToken, normalCount, lineNumber);
            }
        }
    }

    return corner;
}
} // namespace

Model::Model(const std::string& objPath)
{
    std::ifstream file(objPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open OBJ model: " + objPath);
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<ObjTriangle> triangles;

    std::string line;
    size_t lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag.empty() || tag[0] == '#')
        {
            continue;
        }

        if (tag == "v")
        {
            glm::vec3 position(0.0f);
            if (!(stream >> position.x >> position.y >> position.z))
            {
                throw std::runtime_error("OBJ: malformed vertex at line " + std::to_string(lineNumber));
            }
            positions.push_back(position);
        }
        else if (tag == "vt")
        {
            glm::vec2 texCoord(0.0f);
            if (!(stream >> texCoord.x >> texCoord.y))
            {
                throw std::runtime_error("OBJ: malformed texcoord at line " + std::to_string(lineNumber));
            }
            texCoords.push_back(texCoord);
        }
        else if (tag == "vn")
        {
            glm::vec3 normal(0.0f);
            if (!(stream >> normal.x >> normal.y >> normal.z))
            {
                throw std::runtime_error("OBJ: malformed normal at line " + std::to_string(lineNumber));
            }
            normals.push_back(normal);
        }
        else if (tag == "f")
        {
            std::vector<ObjCorner> corners;
            std::string token;

            while (stream >> token)
            {
                corners.push_back(ParseCorner(token,
                                              positions.size(),
                                              texCoords.size(),
                                              normals.size(),
                                              lineNumber));
            }

            if (corners.size() < 3)
            {
                throw std::runtime_error("OBJ: face with fewer than 3 vertices at line " +
                                         std::to_string(lineNumber));
            }

            for (size_t i = 1; i + 1 < corners.size(); ++i)
            {
                triangles.push_back({corners[0], corners[i], corners[i + 1]});
            }
        }
    }

    if (positions.empty() || triangles.empty())
    {
        throw std::runtime_error("OBJ: no usable geometry in " + objPath);
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<size_t> positionRefs;

    vertices.reserve(triangles.size() * 3);
    indices.reserve(triangles.size() * 3);
    positionRefs.reserve(triangles.size() * 3);

    bool needsNormals = false;

    for (const ObjTriangle& triangle : triangles)
    {
        for (const ObjCorner& corner : triangle.corners)
        {
            Vertex vertex;
            vertex.position = positions[corner.position];

            if (corner.texCoord >= 0)
            {
                vertex.texCoord = texCoords[static_cast<size_t>(corner.texCoord)];
            }

            if (corner.normal >= 0)
            {
                vertex.normal = normals[static_cast<size_t>(corner.normal)];
            }
            else
            {
                needsNormals = true;
            }

            vertices.push_back(vertex);
            positionRefs.push_back(static_cast<size_t>(corner.position));
        }
    }

    if (needsNormals)
    {
        std::vector<glm::vec3> accumulated(positions.size(), glm::vec3(0.0f));

        for (size_t i = 0; i + 2 < vertices.size(); i += 3)
        {
            const glm::vec3 edge1 = vertices[i + 1].position - vertices[i].position;
            const glm::vec3 edge2 = vertices[i + 2].position - vertices[i].position;
            const glm::vec3 faceNormal = glm::cross(edge1, edge2);

            accumulated[positionRefs[i]] += faceNormal;
            accumulated[positionRefs[i + 1]] += faceNormal;
            accumulated[positionRefs[i + 2]] += faceNormal;
        }

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            const glm::vec3& normal = accumulated[positionRefs[i]];
            vertices[i].normal = (glm::length(normal) > 0.0f)
                                     ? glm::normalize(normal)
                                     : glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
    else
    {
        for (Vertex& vertex : vertices)
        {
            if (glm::length(vertex.normal) > 0.0f)
            {
                vertex.normal = glm::normalize(vertex.normal);
            }
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        indices.push_back(static_cast<unsigned int>(i));
    }

    const size_t vertexCount = vertices.size();
    const size_t indexCount = indices.size();

    m_Meshes.emplace_back(std::move(vertices), std::move(indices));

    LOG_INFO("Loaded OBJ " << objPath << " | vertices=" << vertexCount
                           << " indices=" << indexCount);
}

void Model::Draw(const Shader& shader) const
{
    for (const Mesh& mesh : m_Meshes)
    {
        mesh.Draw(shader);
    }
}

size_t Model::GetTotalIndexCount() const
{
    size_t total = 0;
    for (const Mesh& mesh : m_Meshes)
    {
        total += mesh.GetIndexCount();
    }
    return total;
}
