#pragma once

#include <glm/glm.hpp>

#include "engine/Renderer/Shader.h"

// Blinn-Phong material parameters.
struct Material
{
    glm::vec3 ambient{0.2f, 0.2f, 0.2f};
    glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
    glm::vec3 specular{0.5f, 0.5f, 0.5f};
    float shininess = 32.0f;

    void Apply(Shader& shader) const
    {
        shader.SetUniform("uMaterial.ambient", ambient);
        shader.SetUniform("uMaterial.diffuse", diffuse);
        shader.SetUniform("uMaterial.specular", specular);
        shader.SetUniform("uMaterial.shininess", shininess);
    }
};

struct DirectionalLight
{
    glm::vec3 direction{-0.2f, -1.0f, -0.3f};
    glm::vec3 ambient{0.1f, 0.1f, 0.1f};
    glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
    glm::vec3 specular{1.0f, 1.0f, 1.0f};

    void Apply(Shader& shader) const
    {
        shader.SetUniform("uDirLight.direction", direction);
        shader.SetUniform("uDirLight.ambient", ambient);
        shader.SetUniform("uDirLight.diffuse", diffuse);
        shader.SetUniform("uDirLight.specular", specular);
    }
};

struct PointLight
{
    glm::vec3 position{0.0f, 2.0f, 2.0f};
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    glm::vec3 ambient{0.05f, 0.05f, 0.05f};
    glm::vec3 diffuse{0.6f, 0.7f, 1.0f};
    glm::vec3 specular{1.0f, 1.0f, 1.0f};

    void Apply(Shader& shader) const
    {
        shader.SetUniform("uPointLight.position", position);
        shader.SetUniform("uPointLight.constant", constant);
        shader.SetUniform("uPointLight.linear", linear);
        shader.SetUniform("uPointLight.quadratic", quadratic);
        shader.SetUniform("uPointLight.ambient", ambient);
        shader.SetUniform("uPointLight.diffuse", diffuse);
        shader.SetUniform("uPointLight.specular", specular);
    }
};
