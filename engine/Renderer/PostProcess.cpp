#include "engine/Renderer/PostProcess.h"

#include <glad/gles2.h>

#include <stdexcept>
#include <utility>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"

namespace
{
const float kFullScreenQuadVertices[] = {
    // positions   // tex coords
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
    -1.0f,  1.0f,   0.0f, 1.0f,
};
} // namespace

PostProcess::PostProcess(unsigned int width, unsigned int height, bool hdrScene)
    : m_FullScreenVertexBuffer(kFullScreenQuadVertices, sizeof(kFullScreenQuadVertices)),
      m_SceneFramebuffer(width, height, hdrScene, false),
      m_PingPongFramebuffers{Framebuffer(width, height, hdrScene, false),
                             Framebuffer(width, height, hdrScene, false)},
      m_HDRScene(hdrScene)
{
    const VertexAttribute attributes[] = {
        {2, GL_FLOAT, false, 4 * sizeof(float), 0},
        {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
    };

    m_FullScreenVertexArray.Bind();
    m_FullScreenVertexArray.AddVertexBuffer(m_FullScreenVertexBuffer, attributes, 2);
    m_FullScreenVertexArray.Unbind();
}

void PostProcess::BeginScene()
{
    m_SceneFramebuffer.Bind();
    GLCall(glViewport(0,
                      0,
                      static_cast<GLsizei>(m_SceneFramebuffer.GetWidth()),
                      static_cast<GLsizei>(m_SceneFramebuffer.GetHeight())));
    GLCall(glEnable(GL_DEPTH_TEST));
}

void PostProcess::EndScene()
{
    Framebuffer::Unbind();
}

void PostProcess::AddEffect(std::unique_ptr<PostEffect> effect, std::shared_ptr<Shader> shader)
{
    if (!effect || !shader)
    {
        throw std::invalid_argument("PostProcess::AddEffect requires an effect and a shader");
    }

    m_Effects.push_back({std::move(effect), std::move(shader), true});
}

void PostProcess::SetEffectEnabled(const std::string& name, bool enabled)
{
    bool found = false;

    for (EffectEntry& entry : m_Effects)
    {
        if (name == entry.Effect->GetName())
        {
            entry.Enabled = enabled;
            found = true;
        }
    }

    if (!found)
    {
        LOG_WARN("PostProcess: unknown effect '" << name << "'");
    }
}

void PostProcess::SetOutputTransform(std::shared_ptr<Shader> shader, bool useHDR)
{
    if (!shader)
    {
        throw std::invalid_argument("PostProcess::SetOutputTransform requires a shader");
    }

    m_OutputShader = std::move(shader);
    m_OutputUseHDR = useHDR;

    // Configure the tonemapping branch once here; the example only has to set
    // the screen/bloom textures before drawing the final full-screen quad.
    m_OutputShader->Bind();
    m_OutputShader->SetUniform("u_UseHDR", useHDR ? 1 : 0);
    m_OutputShader->Unbind();
}

void PostProcess::SetHDRScene(bool hdr)
{
    if (m_HDRScene == hdr)
    {
        return;
    }

    const unsigned int width = m_SceneFramebuffer.GetWidth();
    const unsigned int height = m_SceneFramebuffer.GetHeight();

    if (width == 0 || height == 0)
    {
        m_HDRScene = hdr;
        return;
    }

    // Framebuffer's move assignment releases the old attachments before
    // adopting the new ones, so recreating all three targets is leak-free.
    m_SceneFramebuffer = Framebuffer(width, height, hdr, false);
    m_PingPongFramebuffers[0] = Framebuffer(width, height, hdr, false);
    m_PingPongFramebuffers[1] = Framebuffer(width, height, hdr, false);

    m_HDRScene = hdr;
}

unsigned int PostProcess::Render()
{
    unsigned int inputTexture = m_SceneFramebuffer.GetColorAttachmentID();
    int targetIndex = 0;
    bool applied = false;

    for (EffectEntry& entry : m_Effects)
    {
        if (!entry.Enabled)
        {
            continue;
        }

        Framebuffer& target = m_PingPongFramebuffers[targetIndex];
        target.Bind();
        GLCall(glViewport(0,
                          0,
                          static_cast<GLsizei>(target.GetWidth()),
                          static_cast<GLsizei>(target.GetHeight())));

        entry.Program->Bind();
        entry.Program->SetUniform("u_ScreenTexture", 0);
        inputTexture = entry.Effect->Apply(inputTexture, target, *entry.Program);
        DrawFullScreenQuad();
        entry.Program->Unbind();

        applied = true;
        targetIndex = 1 - targetIndex;
    }

    Framebuffer::Unbind();

    if (!applied)
    {
        return m_SceneFramebuffer.GetColorAttachmentID();
    }

    return inputTexture;
}

void PostProcess::Resize(unsigned int width, unsigned int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    m_SceneFramebuffer.Resize(width, height);
    m_PingPongFramebuffers[0].Resize(width, height);
    m_PingPongFramebuffers[1].Resize(width, height);

    GLCall(glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height)));
}

void PostProcess::DrawFullScreenQuad()
{
    GLCall(glDisable(GL_DEPTH_TEST));
    m_FullScreenVertexArray.Bind();
    GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
    m_FullScreenVertexArray.Unbind();
}
