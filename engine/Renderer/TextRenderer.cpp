#include "engine/Renderer/TextRenderer.h"

#include <stb_easy_font.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>
#include <utility>

#include "engine/Core/GLCall.h"

namespace
{
const char* kTextVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform vec2 uScale;
uniform vec2 uOffset;

out vec4 vColor;

void main()
{
    vColor = aColor;
    vec2 position = aPos.xy * uScale + uOffset;
    gl_Position = uProjection * vec4(position, 0.0, 1.0);
}
)";

const char* kTextFragmentShader = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";

unsigned char ToByte(float value)
{
    return static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}
} // namespace

TextRenderer::TextRenderer()
    : m_Shader(kTextVertexShader, kTextFragmentShader)
{
    m_VertexBuffer.resize(static_cast<size_t>(kMaxQuads) * 64);

    GLCall(glGenVertexArrays(1, &m_VAO));
    GLCall(glGenBuffers(1, &m_VBO));
    GLCall(glGenBuffers(1, &m_EBO));

    GLCall(glBindVertexArray(m_VAO));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO));
    GLCall(glBufferData(GL_ARRAY_BUFFER,
                        static_cast<GLsizeiptr>(m_VertexBuffer.size()),
                        nullptr,
                        GL_DYNAMIC_DRAW));

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, nullptr));
    GLCall(glEnableVertexAttribArray(0));

    GLCall(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16,
                                 reinterpret_cast<const void*>(12)));
    GLCall(glEnableVertexAttribArray(1));

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(kMaxQuads) * 6);
    for (int quad = 0; quad < kMaxQuads; ++quad)
    {
        const unsigned int base = static_cast<unsigned int>(quad * 4);
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                        indices.data(),
                        GL_STATIC_DRAW));

    GLCall(glBindVertexArray(0));
}

TextRenderer::~TextRenderer()
{
    Destroy();
}

TextRenderer::TextRenderer(TextRenderer&& other) noexcept
    : m_Shader(std::move(other.m_Shader)),
      m_VAO(other.m_VAO),
      m_VBO(other.m_VBO),
      m_EBO(other.m_EBO),
      m_VertexBuffer(std::move(other.m_VertexBuffer))
{
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        m_Shader = std::move(other.m_Shader);
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        m_VertexBuffer = std::move(other.m_VertexBuffer);

        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_EBO = 0;
    }
    return *this;
}

void TextRenderer::Draw(const std::string& text,
                        float x,
                        float y,
                        float scale,
                        const glm::vec4& color,
                        int viewportWidth,
                        int viewportHeight)
{
    if (text.empty() || viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    std::vector<char> textCopy(text.begin(), text.end());
    textCopy.push_back('\0');

    unsigned char colorBytes[4] = {
        ToByte(color.r),
        ToByte(color.g),
        ToByte(color.b),
        ToByte(color.a),
    };

    const int numQuads = stb_easy_font_print(0.0f,
                                             0.0f,
                                             textCopy.data(),
                                             colorBytes,
                                             m_VertexBuffer.data(),
                                             static_cast<int>(m_VertexBuffer.size()));

    if (numQuads <= 0)
    {
        return;
    }

    GLboolean depthTestEnabled = GL_FALSE;
    GLCall(depthTestEnabled = glIsEnabled(GL_DEPTH_TEST));
    if (depthTestEnabled == GL_TRUE)
    {
        GLCall(glDisable(GL_DEPTH_TEST));
    }

    const glm::mat4 projection = glm::ortho(0.0f,
                                            static_cast<float>(viewportWidth),
                                            static_cast<float>(viewportHeight),
                                            0.0f,
                                            -1.0f,
                                            1.0f);

    m_Shader.Bind();
    m_Shader.SetUniform("uProjection", projection);
    m_Shader.SetUniform("uScale", glm::vec2(scale, scale));
    m_Shader.SetUniform("uOffset", glm::vec2(x, y));

    GLCall(glBindVertexArray(m_VAO));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBO));
    GLCall(glBufferSubData(GL_ARRAY_BUFFER,
                           0,
                           static_cast<GLsizeiptr>(numQuads) * 64,
                           m_VertexBuffer.data()));

    GLCall(glDrawElements(GL_TRIANGLES,
                          numQuads * 6,
                          GL_UNSIGNED_INT,
                          nullptr));

    GLCall(glBindVertexArray(0));
    m_Shader.Unbind();

    if (depthTestEnabled == GL_TRUE)
    {
        GLCall(glEnable(GL_DEPTH_TEST));
    }
}

void TextRenderer::Destroy()
{
    if (m_EBO != 0)
    {
        GLCall(glDeleteBuffers(1, &m_EBO));
        m_EBO = 0;
    }
    if (m_VBO != 0)
    {
        GLCall(glDeleteBuffers(1, &m_VBO));
        m_VBO = 0;
    }
    if (m_VAO != 0)
    {
        GLCall(glDeleteVertexArrays(1, &m_VAO));
        m_VAO = 0;
    }
}
