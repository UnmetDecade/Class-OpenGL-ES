#pragma once

#include <istream>
#include <string>
#include <vector>

#include "engine/Renderer/Vertex.h"

// Wavefront OBJ parsing without any OpenGL dependency, so the import logic can
// be unit-tested headless. Supports v / vt / vn / f (triangles and polygons),
// negative indices and missing normals. Material files (.mtl), textures and
// smoothing groups are intentionally not parsed.
namespace ObjLoader
{
struct Data
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// Parses from an arbitrary stream. sourceName only appears in error messages.
// Throws std::runtime_error on malformed input or when no geometry is found.
Data Parse(std::istream& stream, const std::string& sourceName);

// Opens objPath and parses it. Throws std::runtime_error when the file cannot
// be opened.
Data Load(const std::string& objPath);
} // namespace ObjLoader
