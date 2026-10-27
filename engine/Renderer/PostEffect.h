#pragma once

#include <string>
#include <utility>

#include "engine/Core/GLCall.h"
#include "engine/Renderer/Framebuffer.h"

class Shader;

// Base class for one pass of the post-processing chain.
class PostEffect
{
public:
    virtual ~PostEffect() = default;

    virtual const char* GetName() const = 0;

    // Binds inputTex to texture unit 0. PostProcess then sets u_ScreenTexture
    // on shader and draws the full-screen quad into target (which the caller
    // has already bound). Returns the texture this pass produced.
    virtual unsigned int Apply(unsigned int inputTex,
                               Framebuffer& target,
                               const Shader& shader) = 0;
};

// Concrete effect backed by a single fragment shader and a display name.
class ShaderPostEffect final : public PostEffect
{
public:
    explicit ShaderPostEffect(std::string name) : m_Name(std::move(name)) {}

    const char* GetName() const override { return m_Name.c_str(); }

    unsigned int Apply(unsigned int inputTex,
                       Framebuffer& target,
                       const Shader& shader) override
    {
        (void)shader; // u_ScreenTexture is set by PostProcess before the draw.

        GLCall(glActiveTexture(GL_TEXTURE0));
        GLCall(glBindTexture(GL_TEXTURE_2D, inputTex));
        return target.GetColorAttachmentID();
    }

private:
    std::string m_Name;
};
