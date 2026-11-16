#include "engine/Renderer/Model.h"

#include <utility>

#include "engine/Core/Log.h"
#include "engine/Renderer/ObjLoader.h"
#include "engine/Renderer/Shader.h"

Model::Model(const std::string& objPath)
{
    ObjLoader::Data data = ObjLoader::Load(objPath);
    const size_t vertexCount = data.vertices.size();
    const size_t indexCount = data.indices.size();

    m_Meshes.emplace_back(std::move(data.vertices), std::move(data.indices));

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
