#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/Renderer/Shader.h"

// Owns shaders by name and loads them from single-file GLSL sources using
// "#type vertex" / "#type fragment" markers.
class ShaderLibrary
{
public:
    void Add(const std::string& name, const std::shared_ptr<Shader>& shader);

    // The shader name is derived from the file stem.
    std::shared_ptr<Shader> Load(const std::string& filePath);
    std::shared_ptr<Shader> Load(const std::string& name, const std::string& filePath);

    std::shared_ptr<Shader> Get(const std::string& name);
    bool Exists(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};
