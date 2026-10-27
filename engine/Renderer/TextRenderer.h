#pragma once

#include <glad/gles2.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "engine/Renderer/Shader.h"

// Screen-space ASCII text renderer built on stb_easy_font. Coordinates and
// sizes are in framebuffer pixels with the origin at the top-left.
class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    TextRenderer(TextRenderer&& other) noexcept;
    TextRenderer& operator=(TextRenderer&& other) noexcept;

    void Draw(const std::string& text,
              float x,
              float y,
              float scale,
              const glm::vec4& color,
              int viewportWidth,
              int viewportHeight);

private:
    static constexpr int kMaxQuads = 4096;

    void Destroy();

    Shader m_Shader;
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    std::vector<char> m_VertexBuffer;
};
