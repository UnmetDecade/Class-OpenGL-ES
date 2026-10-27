#include "engine/Renderer/ShadowMap.h"

#include <glad/gles2.h>

#include <sstream>
#include <stdexcept>

#include "engine/Core/GLCall.h"

ShadowMap::ShadowMap(unsigned int resolution)
    : m_Resolution(resolution)
{
    GLCall(glGenFramebuffers(1, &m_FBO));

    GLCall(glGenTextures(1, &m_DepthTexture));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_DepthTexture));
    // OpenGL ES 3.0 has no GL_CLAMP_TO_BORDER / GL_TEXTURE_BORDER_COLOR, so
    // edge clamping is used instead. The shadow shader already rejects
    // fragments outside the light frustum, which makes the border irrelevant.
    GLCall(glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_DEPTH_COMPONENT24,
                        static_cast<GLsizei>(m_Resolution),
                        static_cast<GLsizei>(m_Resolution),
                        0,
                        GL_DEPTH_COMPONENT,
                        GL_UNSIGNED_INT,
                        nullptr));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FBO));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  GL_TEXTURE_2D,
                                  m_DepthTexture,
                                  0));
    const GLenum noColorAttachment = GL_NONE;
    GLCall(glDrawBuffers(1, &noColorAttachment));
    GLCall(glReadBuffer(GL_NONE));

    GLenum status = GL_FRAMEBUFFER_UNDEFINED;
    GLCall(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        GLCall(glDeleteTextures(1, &m_DepthTexture));
        m_DepthTexture = 0;
        GLCall(glDeleteFramebuffers(1, &m_FBO));
        m_FBO = 0;

        std::ostringstream message;
        message << "ShadowMap framebuffer is incomplete (" << m_Resolution << "x"
                << m_Resolution << "): status 0x" << std::hex << status;
        throw std::runtime_error(message.str());
    }
}

ShadowMap::~ShadowMap()
{
    if (m_FBO != 0)
    {
        GLCall(glDeleteFramebuffers(1, &m_FBO));
        m_FBO = 0;
    }
    if (m_DepthTexture != 0)
    {
        GLCall(glDeleteTextures(1, &m_DepthTexture));
        m_DepthTexture = 0;
    }
}

ShadowMap::ShadowMap(ShadowMap&& other) noexcept
    : m_FBO(other.m_FBO),
      m_DepthTexture(other.m_DepthTexture),
      m_Resolution(other.m_Resolution),
      m_LightSpace(other.m_LightSpace)
{
    other.m_FBO = 0;
    other.m_DepthTexture = 0;
    other.m_Resolution = 0;
    other.m_LightSpace = glm::mat4(1.0f);
}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept
{
    if (this != &other)
    {
        if (m_FBO != 0)
        {
            GLCall(glDeleteFramebuffers(1, &m_FBO));
        }
        if (m_DepthTexture != 0)
        {
            GLCall(glDeleteTextures(1, &m_DepthTexture));
        }

        m_FBO = other.m_FBO;
        m_DepthTexture = other.m_DepthTexture;
        m_Resolution = other.m_Resolution;
        m_LightSpace = other.m_LightSpace;

        other.m_FBO = 0;
        other.m_DepthTexture = 0;
        other.m_Resolution = 0;
        other.m_LightSpace = glm::mat4(1.0f);
    }
    return *this;
}

void ShadowMap::BeginWrite(const glm::mat4& lightSpaceMatrix)
{
    m_LightSpace = lightSpaceMatrix;

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FBO));
    GLCall(glViewport(0, 0,
                      static_cast<GLsizei>(m_Resolution),
                      static_cast<GLsizei>(m_Resolution)));
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void ShadowMap::EndWrite(unsigned int windowWidth, unsigned int windowHeight)
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GLCall(glViewport(0, 0,
                      static_cast<GLsizei>(windowWidth),
                      static_cast<GLsizei>(windowHeight)));
}

void ShadowMap::BindForReading(unsigned int slot) const
{
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_DepthTexture));
}

unsigned int ShadowMap::GetDepthTexture() const
{
    return m_DepthTexture;
}

unsigned int ShadowMap::GetResolution() const
{
    return m_Resolution;
}
