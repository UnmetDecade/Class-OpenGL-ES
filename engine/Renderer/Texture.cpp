#include "engine/Renderer/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <utility>

#include "engine/Core/GLCall.h"

Texture::Texture(const std::string& path)
{
    stbi_set_flip_vertically_on_load(1);

    // Always decode to RGBA. On OpenGL ES, glGenerateMipmap rejects the
    // 3-component GL_SRGB8 format on some drivers (Mesa returns
    // GL_INVALID_OPERATION), while GL_SRGB8_ALPHA8 works everywhere. It also
    // removes any row-alignment concerns for odd-width RGB images.
    int fileChannels = 0;
    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &fileChannels, 4);
    if (data == nullptr)
    {
        throw std::runtime_error("Failed to load texture: " + path + " (" + stbi_failure_reason() + ")");
    }
    m_Channels = 4;

    // Image files are authored in sRGB. The sRGB internal formats make the
    // GPU convert sRGB -> linear on sampling, so all lighting math happens in
    // linear space; the inverse conversion happens once in the final output
    // pass (see hdr_output.frag).
    const GLenum format = GL_RGBA;
    const GLenum internalFormat = GL_SRGB8_ALPHA8;

    GLCall(glGenTextures(1, &m_ID));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    GLCall(glTexImage2D(GL_TEXTURE_2D,
                        0,
                        internalFormat,
                        m_Width,
                        m_Height,
                        0,
                        format,
                        GL_UNSIGNED_BYTE,
                        data));
    GLCall(glGenerateMipmap(GL_TEXTURE_2D));

    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    stbi_image_free(data);
}

Texture::~Texture()
{
    if (m_ID != 0)
    {
        GLCall(glDeleteTextures(1, &m_ID));
        m_ID = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_ID(other.m_ID),
      m_Width(other.m_Width),
      m_Height(other.m_Height),
      m_Channels(other.m_Channels)
{
    other.m_ID = 0;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Channels = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_ID != 0)
        {
            GLCall(glDeleteTextures(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Channels = other.m_Channels;

        other.m_ID = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Channels = 0;
    }
    return *this;
}

void Texture::Bind(unsigned int slot) const
{
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));
}

void Texture::Unbind() const
{
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}
