#pragma once

#include <glm/glm.hpp>

// Plain vertex layout shared by every mesh-producing module. Kept free of
// OpenGL headers so parsers and tests can use it without a GL context.
struct Vertex
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 0.0f};
    glm::vec2 texCoord{0.0f, 0.0f};
};
