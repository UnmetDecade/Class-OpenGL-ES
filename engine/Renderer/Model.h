#pragma once

#include <string>
#include <vector>

#include "engine/Renderer/Mesh.h"

class Shader;

// Minimal Wavefront OBJ importer: supports v / vt / vn / f (triangles and
// polygons), negative indices and missing normals. Material files (.mtl),
// textures and smoothing groups are intentionally not parsed.
class Model
{
public:
    explicit Model(const std::string& objPath);

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    void Draw(const Shader& shader) const;

    size_t GetMeshCount() const { return m_Meshes.size(); }
    size_t GetTotalIndexCount() const;

private:
    std::vector<Mesh> m_Meshes;
};
