#pragma once

#include <array>
#include <string>

// RAII wrapper around an OpenGL cubemap texture loaded from six face images.
class Cubemap
{
public:
    // Face order is fixed: +X, -X, +Y, -Y, +Z, -Z.
    explicit Cubemap(const std::array<std::string, 6>& facePaths);
    ~Cubemap();

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    Cubemap(Cubemap&& other) noexcept;
    Cubemap& operator=(Cubemap&& other) noexcept;

    void Bind(unsigned int slot = 0) const;
    unsigned int GetRendererID() const { return m_RendererID; }

private:
    unsigned int m_RendererID = 0;
};
