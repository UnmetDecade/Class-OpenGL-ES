#pragma once

#include <string>

#include <glm/glm.hpp>

#include "engine/Renderer/Shader.h"

// Metallic-roughness PBR material parameters. Albedo is authored in linear
// space; Metallic/Roughness/AO are in [0, 1].
struct MaterialPBR
{
    glm::vec3 Albedo{0.8f};
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float AO = 1.0f;

    // Sets u_Material.Albedo / .Metallic / .Roughness / .AO.
    //
    // Shader::SetUniform is not const because it caches uniform locations;
    // that mutation is logically const (the program itself is unchanged), so
    // the const reference from the interface can be used safely.
    void Apply(const Shader& shader, const std::string& prefix = "u_Material") const
    {
        Shader& mutableShader = const_cast<Shader&>(shader);

        mutableShader.SetUniform(prefix + ".Albedo", Albedo);
        mutableShader.SetUniform(prefix + ".Metallic", Metallic);
        mutableShader.SetUniform(prefix + ".Roughness", Roughness);
        mutableShader.SetUniform(prefix + ".AO", AO);
    }
};
