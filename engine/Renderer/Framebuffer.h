#pragma once

// RAII wrapper around an off-screen render target: a color texture attachment
// plus a packed depth/stencil renderbuffer.
//
// Color formats:
//   hdr=false, sRGBColorSpace=false -> GL_RGB8     (default, legacy behavior)
//   hdr=false, sRGBColorSpace=true  -> GL_SRGB8    (GPU encodes linear -> sRGB
//                                                   when the framebuffer is
//                                                   written; sampling decodes)
//   hdr=true                        -> GL_RGBA16F  (linear, values above 1.0
//                                                   are preserved)
class Framebuffer
{
public:
    // width/height are the attachment resolution. The color texture and the
    // depth/stencil renderbuffer are created by Invalidate().
    Framebuffer(unsigned int width, unsigned int height,
                bool hdr = false,
                bool sRGBColorSpace = false);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void Bind() const;
    static void Unbind();
    void Resize(unsigned int width, unsigned int height);

    unsigned int GetColorAttachmentID() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    bool IsHDR() const { return m_IsHDR; }

private:
    void Invalidate();

    unsigned int m_RendererID = 0;
    unsigned int m_ColorAttachment = 0;
    unsigned int m_DepthAttachment = 0;
    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
    bool m_IsHDR = false;
    // Only meaningful when m_IsHDR is false.
    bool m_IsSRGB = false;
};
