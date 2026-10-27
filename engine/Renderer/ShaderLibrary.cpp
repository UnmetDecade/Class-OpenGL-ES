#include "engine/Renderer/ShaderLibrary.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "engine/Core/Log.h"

namespace
{
enum class ShaderType
{
    None,
    Vertex,
    Fragment,
};

std::string ReadWholeFile(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

std::string FileStem(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    const size_t start = (slash == std::string::npos) ? 0 : slash + 1;
    const size_t dot = path.find_last_of('.');

    if (dot == std::string::npos || dot < start)
    {
        return path.substr(start);
    }
    return path.substr(start, dot - start);
}

struct ShaderSources
{
    std::string vertex;
    std::string fragment;
};

ShaderSources ParseShaderFile(const std::string& filePath)
{
    const std::string text = ReadWholeFile(filePath);

    ShaderSources sources;
    ShaderType currentType = ShaderType::None;

    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.find("#type vertex") != std::string::npos)
        {
            currentType = ShaderType::Vertex;
            continue;
        }
        if (line.find("#type fragment") != std::string::npos)
        {
            currentType = ShaderType::Fragment;
            continue;
        }

        switch (currentType)
        {
        case ShaderType::Vertex:
            sources.vertex += line + '\n';
            break;
        case ShaderType::Fragment:
            sources.fragment += line + '\n';
            break;
        case ShaderType::None:
            break;
        }
    }

    if (sources.vertex.empty() || sources.fragment.empty())
    {
        throw std::runtime_error("Shader file is missing '#type vertex' or '#type fragment': " + filePath);
    }

    return sources;
}
} // namespace

void ShaderLibrary::Add(const std::string& name, const std::shared_ptr<Shader>& shader)
{
    if (Exists(name))
    {
        LOG_WARN("ShaderLibrary: replacing existing shader '" << name << "'");
    }
    m_Shaders[name] = shader;
}

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& filePath)
{
    return Load(FileStem(filePath), filePath);
}

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filePath)
{
    const ShaderSources sources = ParseShaderFile(filePath);

    auto shader = std::make_shared<Shader>(sources.vertex.c_str(), sources.fragment.c_str());
    Add(name, shader);
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name)
{
    const auto it = m_Shaders.find(name);
    if (it == m_Shaders.end())
    {
        throw std::runtime_error("ShaderLibrary: shader not found: " + name);
    }
    return it->second;
}

bool ShaderLibrary::Exists(const std::string& name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}
