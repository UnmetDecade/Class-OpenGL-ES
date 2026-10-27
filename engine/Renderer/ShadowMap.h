#pragma once

#include <glm/mat4x4.hpp>

// RAII wrapper around a depth-only framebuffer used as a directional light's
// shadow map. The depth texture is sampled manually in the lighting shader
// (no hardware compare mode).
class ShadowMap
{
public:
    explicit ShadowMap(unsigned int resolution = 2048);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    ShadowMap(ShadowMap&& other) noexcept;
    ShadowMap& operator=(ShadowMap&& other) noexcept;

    // Binds the depth FBO, sets the light-space viewport and clears depth.
    void BeginWrite(const glm::mat4& lightSpaceMatrix);
    // Restores the default framebuffer and the window viewport.
    void EndWrite(unsigned int windowWidth, unsigned int windowHeight);

    void BindForReading(unsigned int slot = 1) const;

    unsigned int GetDepthTexture() const;
    unsigned int GetResolution() const;

private:
    unsigned int m_FBO = 0;
    unsigned int m_DepthTexture = 0;
    unsigned int m_Resolution;

    // Cached from BeginWrite for callers that need the light matrices.
    glm::mat4 m_LightSpace{1.0f};
};
