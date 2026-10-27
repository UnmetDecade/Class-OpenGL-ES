#include "engine/Renderer/Framebuffer.h"

#include <glad/gles2.h>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"

namespace
{
const char* FramebufferStatusToString(GLenum status)
{
    switch (status)
    {
    case GL_FRAMEBUFFER_COMPLETE:                      return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_UNDEFINED:                     return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:        return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:        return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
#endif
    case GL_FRAMEBUFFER_UNSUPPORTED:                   return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default:                                           return "GL_FRAMEBUFFER_UNKNOWN";
    }
}

// Rendering to 16-bit float color attachments needs EXT_color_buffer_float on
// OpenGL ES 3.0/3.1. Without it the HDR path silently falls back to RGBA8,
// which clamps at 1.0 but still produces a correct (if less bright) image.
bool SupportsFloatColorBuffers()
{
    static const bool supported = []
    {
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        return extensions != nullptr && std::strstr(extensions, "GL_EXT_color_buffer_float") != nullptr;
    }();
    return supported;
}
} // namespace

Framebuffer::Framebuffer(unsigned int width, unsigned int height,
                         bool hdr,
                         bool sRGBColorSpace)
    : m_Width(width),
      m_Height(height),
      m_IsHDR(hdr),
      m_IsSRGB(sRGBColorSpace)
{
    Invalidate();
}

Framebuffer::~Framebuffer()
{
    if (m_RendererID != 0)
    {
        GLCall(glDeleteFramebuffers(1, &m_RendererID));
        m_RendererID = 0;
    }
    if (m_ColorAttachment != 0)
    {
        GLCall(glDeleteTextures(1, &m_ColorAttachment));
        m_ColorAttachment = 0;
    }
    if (m_DepthAttachment != 0)
    {
        GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
        m_DepthAttachment = 0;
    }
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_RendererID(other.m_RendererID),
      m_ColorAttachment(other.m_ColorAttachment),
      m_DepthAttachment(other.m_DepthAttachment),
      m_Width(other.m_Width),
      m_Height(other.m_Height),
      m_IsHDR(other.m_IsHDR),
      m_IsSRGB(other.m_IsSRGB)
{
    other.m_RendererID = 0;
    other.m_ColorAttachment = 0;
    other.m_DepthAttachment = 0;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_IsHDR = false;
    other.m_IsSRGB = false;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        // Release the attachments owned by this framebuffer before taking
        // over the source's handles, so no GL object leaks.
        if (m_RendererID != 0)
        {
            GLCall(glDeleteFramebuffers(1, &m_RendererID));
        }
        if (m_ColorAttachment != 0)
        {
            GLCall(glDeleteTextures(1, &m_ColorAttachment));
        }
        if (m_DepthAttachment != 0)
        {
            GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
        }

        m_RendererID = other.m_RendererID;
        m_ColorAttachment = other.m_ColorAttachment;
        m_DepthAttachment = other.m_DepthAttachment;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_IsHDR = other.m_IsHDR;
        m_IsSRGB = other.m_IsSRGB;

        other.m_RendererID = 0;
        other.m_ColorAttachment = 0;
        other.m_DepthAttachment = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_IsHDR = false;
        other.m_IsSRGB = false;
    }
    return *this;
}

void Framebuffer::Bind() const
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));
}

void Framebuffer::Unbind()
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::Resize(unsigned int width, unsigned int height)
{
    if (width == 0 || height == 0)
    {
        // Minimized windows report a zero-sized framebuffer; keep the last
        // valid attachments instead of creating an incomplete FBO.
        return;
    }

    if (width == m_Width && height == m_Height)
    {
        return;
    }

    if (m_RendererID != 0)
    {
        GLCall(glDeleteFramebuffers(1, &m_RendererID));
        m_RendererID = 0;
    }
    if (m_ColorAttachment != 0)
    {
        GLCall(glDeleteTextures(1, &m_ColorAttachment));
        m_ColorAttachment = 0;
    }
    if (m_DepthAttachment != 0)
    {
        GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
        m_DepthAttachment = 0;
    }

    m_Width = width;
    m_Height = height;
    Invalidate();
}

unsigned int Framebuffer::GetColorAttachmentID() const
{
    return m_ColorAttachment;
}

unsigned int Framebuffer::GetWidth() const
{
    return m_Width;
}

unsigned int Framebuffer::GetHeight() const
{
    return m_Height;
}

void Framebuffer::Invalidate()
{
    GLCall(glGenFramebuffers(1, &m_RendererID));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));

    GLenum internalFormat = GL_RGB8;
    GLenum format = GL_RGB;
    GLenum type = GL_UNSIGNED_BYTE;

    if (m_IsHDR)
    {
        if (SupportsFloatColorBuffers())
        {
            internalFormat = GL_RGBA16F;
            format = GL_RGBA;
            type = GL_FLOAT;
        }
        else
        {
            LOG_WARN("GL_EXT_color_buffer_float unavailable; HDR framebuffer falls back to RGBA8");
            internalFormat = GL_RGBA8;
        }
    }
    else if (m_IsSRGB)
    {
        internalFormat = GL_SRGB8;
    }

    GLCall(glGenTextures(1, &m_ColorAttachment));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ColorAttachment));
    GLCall(glTexImage2D(GL_TEXTURE_2D,
                        0,
                        internalFormat,
                        static_cast<GLsizei>(m_Width),
                        static_cast<GLsizei>(m_Height),
                        0,
                        format,
                        type,
                        nullptr));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                  GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D,
                                  m_ColorAttachment,
                                  0));

    GLCall(glGenRenderbuffers(1, &m_DepthAttachment));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER,
                                 GL_DEPTH24_STENCIL8,
                                 static_cast<GLsizei>(m_Width),
                                 static_cast<GLsizei>(m_Height)));
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                     GL_DEPTH_STENCIL_ATTACHMENT,
                                     GL_RENDERBUFFER,
                                     m_DepthAttachment));

    GLenum status = GL_FRAMEBUFFER_UNDEFINED;
    GLCall(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        GLCall(glDeleteRenderbuffers(1, &m_DepthAttachment));
        m_DepthAttachment = 0;
        GLCall(glDeleteTextures(1, &m_ColorAttachment));
        m_ColorAttachment = 0;
        GLCall(glDeleteFramebuffers(1, &m_RendererID));
        m_RendererID = 0;

        std::ostringstream message;
        message << "Framebuffer is incomplete (" << m_Width << "x" << m_Height << "): "
                << FramebufferStatusToString(status)
                << " (0x" << std::hex << status << ")";
        throw std::runtime_error(message.str());
    }
}
