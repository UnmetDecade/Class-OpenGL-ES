#pragma once

#include <glad/gles2.h>

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

// RAII wrapper around a linked OpenGL program.
class Shader
{
public:
    // Loads and compiles GLSL from files.
    Shader(const std::string& vertPath, const std::string& fragPath);

    // Compiles the given GLSL sources directly.
    Shader(const char* vertSrc, const char* fragSrc);

    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void Bind() const;
    void Unbind() const;

    void SetUniform(const std::string& name, int value);
    void SetUniform(const std::string& name, float value);

    void SetUniform(const std::string& name, const glm::vec2& value);
    void SetUniform(const std::string& name, const glm::vec3& value);
    void SetUniform(const std::string& name, const glm::vec4& value);
    void SetUniform(const std::string& name, const glm::mat3& value, bool transpose = false);
    void SetUniform(const std::string& name, const glm::mat4& value, bool transpose = false);

    GLuint GetID() const { return m_ID; }

private:
    void Init(const char* vertSrc, const char* fragSrc);

    static std::string ReadFile(const std::string& path);
    static GLuint CompileStage(GLenum type, const char* source, const char* stageName);

    GLint GetUniformLocation(const std::string& name);

    GLuint m_ID = 0;
    std::unordered_map<std::string, GLint> m_UniformCache;
};
