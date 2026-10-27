#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/Renderer/Buffer.h"
#include "engine/Renderer/Framebuffer.h"
#include "engine/Renderer/PostEffect.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/VertexArray.h"

// Renders a scene into an off-screen Framebuffer and runs an ordered chain of
// post-processing effects with ping-pong targets before returning the final
// texture to the caller.
class PostProcess
{
public:
    // hdrScene=false keeps the legacy RGB8 scene target; true creates
    // RGBA16F scene and ping-pong targets so values above 1.0 survive the
    // whole chain.
    PostProcess(unsigned int width, unsigned int height, bool hdrScene = false);
    ~PostProcess() = default;

    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    PostProcess(PostProcess&& other) noexcept = default;
    PostProcess& operator=(PostProcess&& other) noexcept = default;

    // Binds the scene FBO and sets the viewport to its resolution.
    void BeginScene();
    // Unbinds the scene FBO (restores the default framebuffer).
    void EndScene();

    void AddEffect(std::unique_ptr<PostEffect> effect, std::shared_ptr<Shader> shader);
    void SetEffectEnabled(const std::string& name, bool enabled);

    // Records the shader used by the final output pass and configures its
    // u_UseHDR uniform (1 = Reinhard + gamma, 0 = pass-through). The example
    // still issues the full-screen draw itself after Render().
    void SetOutputTransform(std::shared_ptr<Shader> shader, bool useHDR);

    // Recreates the scene and ping-pong targets with the requested color
    // format, keeping the current resolution. Existing effects are preserved.
    void SetHDRScene(bool hdr);
    bool IsHDRScene() const { return m_HDRScene; }

    // Runs every enabled effect in insertion order and returns the texture
    // holding the final image. With no enabled effects this is the scene color
    // attachment.
    unsigned int Render();

    void Resize(unsigned int width, unsigned int height);

private:
    struct EffectEntry
    {
        std::unique_ptr<PostEffect> Effect;
        // Named Program, not Shader: a member sharing the type's name is
        // rejected by GCC ("changes meaning of 'Shader'").
        std::shared_ptr<Shader> Program;
        bool Enabled = true;
    };

    void DrawFullScreenQuad();

    // Declared before the framebuffers so it can be constructed from the
    // static quad data in the initializer list.
    VertexBuffer m_FullScreenVertexBuffer;
    VertexArray m_FullScreenVertexArray;
    Framebuffer m_SceneFramebuffer;
    Framebuffer m_PingPongFramebuffers[2];
    bool m_HDRScene = false;
    std::shared_ptr<Shader> m_OutputShader;
    bool m_OutputUseHDR = false;
    std::vector<EffectEntry> m_Effects;
};
