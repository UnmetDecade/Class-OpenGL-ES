#pragma once

#include <glad/gles2.h>

#include <string>

// RAII wrapper around a 2D texture loaded from an image file.
class Texture
{
public:
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    GLuint GetID() const { return m_ID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetChannels() const { return m_Channels; }

private:
    GLuint m_ID = 0;
    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
};
