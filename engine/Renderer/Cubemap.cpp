#include "engine/Renderer/Cubemap.h"

#include <glad/gles2.h>

// The implementation is provided by Texture.cpp; defining it here as well
// would produce duplicate stb_image symbols at link time.
#include <stb_image.h>

#include <cstddef>
#include <stdexcept>

#include "engine/Core/GLCall.h"

Cubemap::Cubemap(const std::array<std::string, 6>& facePaths)
{
    GLCall(glGenTextures(1, &m_RendererID));
    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));

    // stbi's flip flag is global and Texture enables it; cubemap faces are
    // authored with the top row pointing at +Y, so they must load unflipped.
    stbi_set_flip_vertically_on_load(0);

    for (std::size_t i = 0; i < facePaths.size(); ++i)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        unsigned char* data = stbi_load(facePaths[i].c_str(), &width, &height, &channels, 0);
        if (data == nullptr)
        {
            const std::string reason = stbi_failure_reason();

            GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
            GLCall(glDeleteTextures(1, &m_RendererID));
            m_RendererID = 0;

            throw std::runtime_error("Failed to load cubemap face: " + facePaths[i] +
                                     " (" + reason + ")");
        }

        const GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        // Faces are authored in sRGB; sampling converts them to linear so the
        // lighting/environment math stays in linear space.
        const GLenum internalFormat = (channels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;

        GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i),
                            0,
                            internalFormat,
                            width,
                            height,
                            0,
                            format,
                            GL_UNSIGNED_BYTE,
                            data));

        stbi_image_free(data);
    }

    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

Cubemap::~Cubemap()
{
    if (m_RendererID != 0)
    {
        GLCall(glDeleteTextures(1, &m_RendererID));
        m_RendererID = 0;
    }
}

Cubemap::Cubemap(Cubemap&& other) noexcept
    : m_RendererID(other.m_RendererID)
{
    other.m_RendererID = 0;
}

Cubemap& Cubemap::operator=(Cubemap&& other) noexcept
{
    if (this != &other)
    {
        if (m_RendererID != 0)
        {
            GLCall(glDeleteTextures(1, &m_RendererID));
        }

        m_RendererID = other.m_RendererID;
        other.m_RendererID = 0;
    }
    return *this;
}

void Cubemap::Bind(unsigned int slot) const
{
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));
}
