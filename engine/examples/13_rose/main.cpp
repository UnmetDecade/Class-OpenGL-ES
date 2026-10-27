#include <glad/gles2.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "engine/Core/GLCall.h"
#include "engine/Core/Log.h"
#include "engine/Renderer/Camera.h"
#include "engine/Renderer/Mesh.h"
#include "engine/Renderer/Renderer.h"
#include "engine/Renderer/Shader.h"
#include "engine/Renderer/ShadowMap.h"
#include "engine/Renderer/VertexArray.h"
#include "engine/Window/Window.h"

// ---------------------------------------------------------------------------
// 13_rose - a procedurally generated rose.
//
// The flower head is assembled from ~60 parametric petal surfaces arranged on
// a golden-angle spiral: the innermost petals stand almost upright and curl
// tightly around the bud, while each successive whorl grows wider and leans
// further outwards until the outermost petals reflex back.  The stem, its
// gentle S-curve, thorns, leaves and sepals are generated with the same
// techniques.  Everything is drawn with a single two-sided, translucency-aware
// shader and a procedural gradient backdrop.
// ---------------------------------------------------------------------------

namespace
{
constexpr float kGoldenAngle = 2.39996323f; // radians, 137.5 degrees
constexpr float kStemBottomY = -0.42f;
constexpr float kHeadBaseY = 1.22f;

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// Cheap deterministic hash in [0, 1), used for per-petal variation.
float Hash01(std::uint32_t value)
{
    value = (value << 13u) ^ value;
    value = value * (value * value * 15731u + 789221u) + 1376312589u;
    return static_cast<float>(value & 0x7fffffffu) /
           static_cast<float>(0x7fffffffu);
}

void ComputeNormals(MeshData& mesh)
{
    for (Vertex& vertex : mesh.vertices)
    {
        vertex.normal = glm::vec3(0.0f);
    }

    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
        const unsigned int i0 = mesh.indices[i];
        const unsigned int i1 = mesh.indices[i + 1];
        const unsigned int i2 = mesh.indices[i + 2];

        const glm::vec3 p0 = mesh.vertices[i0].position;
        const glm::vec3 p1 = mesh.vertices[i1].position;
        const glm::vec3 p2 = mesh.vertices[i2].position;

        // Unnormalised cross product = area-weighted face normal, which makes
        // the accumulation below produce smooth, well proportioned normals.
        const glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

        mesh.vertices[i0].normal += faceNormal;
        mesh.vertices[i1].normal += faceNormal;
        mesh.vertices[i2].normal += faceNormal;
    }

    for (Vertex& vertex : mesh.vertices)
    {
        const float length = glm::length(vertex.normal);
        vertex.normal = length > 1e-8f ? vertex.normal / length
                                       : glm::vec3(0.0f, 0.0f, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// Petals
// ---------------------------------------------------------------------------

// A petal patch parameterised by s (base -> tip) and t (-1 -> 1 across the
// petal).  The petal grows along +Y and its outer face looks towards +Z.
// `cup` curls the edges towards -Z (into the flower), `bend` bows the midrib
// outwards, `twist` gives the tip a gentle spiral, and `phase` decorrelates
// the ruffle of neighbouring petals.
glm::vec3 EvalPetal(float s, float t, float height, float width, float cup,
                    float bend, float twist, float phase, float reflex)
{
    // The petal outline is an ellipse standing on its lower tip: that tip is
    // the attachment point, the top is broad and round, and the two ends of
    // the boundary meet at the base without sharp side corners.
    const float ang = glm::pi<float>() * t;
    const float ex = 0.5f * width * std::sin(ang);
    const float ey = 0.5f * height * (1.0f + std::cos(ang));

    // Vertical fraction (0 at the base, 1 at the tip) and cross-petal
    // coordinate (0 on the midrib, +/-1 at the edges).
    const float sv = ey / height;
    const float u = s * std::sin(ang);

    float x = s * ex;
    float y = s * ey;

    // The outer petals arch over and curl back down towards the stem.
    y -= reflex * height * std::pow(sv, 2.2f) * s;

    // Soft heart-shaped notch in the middle of the rim.
    const float notchBlend = sv * sv * (3.0f - 2.0f * sv);
    y -= 0.015f * height * notchBlend * std::exp(-std::pow(u * 2.4f, 2.0f));

    // Cup the petal towards the flower axis and add a gentle ruffle along
    // its edges so the silhouette never reads as a flat sheet.
    const float cupShape = cup * (0.15f + 0.85f * sv) * u * u;
    const float ruffle =
        0.070f * cup * std::sin(t * 5.0f + s * 3.5f + phase) * std::abs(u) * sv;
    const float fineRuffle = 0.022f * cup *
                             std::sin(t * 11.0f - s * 3.0f + phase * 1.7f) *
                             std::abs(u) * sv * sv;

    // A soft raised midrib breaks up the otherwise uniform concavity.
    const float midrib =
        0.065f * cup * std::exp(-std::pow(u * 3.5f, 2.0f)) * sv;

    const float z = bend * s * s - cupShape + ruffle + fineRuffle + midrib;

    // Give the tip a gentle spiral around the petal's own axis.
    const float angle = twist * s * s;
    return glm::vec3(x * std::cos(angle) - z * std::sin(angle), y,
                     x * std::sin(angle) + z * std::cos(angle));
}

void AppendPetal(MeshData& mesh, const glm::mat4& model, float height,
                 float width, float cup, float bend, float twist, float phase,
                 float reflex)
{
    constexpr int kSegmentsS = 34;
    constexpr int kSegmentsT = 30;

    // Petals are grown as a thin closed shell rather than a zero-thickness
    // sheet.  A real petal is only a millimetre or so thick, but that sliver
    // is what gives the rim a rounded, velvety highlight instead of the razor
    // edge that made the folds read as folded paper.
    constexpr float kHalfThickness = 0.0011f;

    constexpr int kStride = kSegmentsT + 1;
    constexpr int kGridCount = (kSegmentsS + 1) * kStride;

    std::vector<glm::vec3> positions(kGridCount);
    std::vector<glm::vec3> normals(kGridCount);
    std::vector<glm::vec2> texCoords(kGridCount);

    const auto evalWorld = [&](float s, float t)
    {
        const glm::vec3 local =
            EvalPetal(s, t, height, width, cup, bend, twist, phase, reflex);
        return glm::vec3(model * glm::vec4(local, 1.0f));
    };

    // Mid-surface positions and normals, from central differences.  The base
    // of the petal collapses to a single point, so the differences are taken
    // one-sided there.
    constexpr float kStep = 0.010f;
    for (int i = 0; i <= kSegmentsS; ++i)
    {
        const float s = static_cast<float>(i) / static_cast<float>(kSegmentsS);

        for (int j = 0; j <= kSegmentsT; ++j)
        {
            const float t = -1.0f +
                            2.0f * static_cast<float>(j) /
                                static_cast<float>(kSegmentsT);

            const glm::vec3 p = evalWorld(s, t);
            const float s0 = std::max(0.0f, s - kStep);
            const float s1 = std::min(1.0f, s + kStep);
            const float t0 = std::max(-1.0f, t - kStep);
            const float t1 = std::min(1.0f, t + kStep);

            const glm::vec3 alongS = evalWorld(s1, t) - evalWorld(s0, t);
            const glm::vec3 alongT = evalWorld(s, t1) - evalWorld(s, t0);

            glm::vec3 n = glm::cross(alongT, alongS);
            const float length = glm::length(n);
            n = length > 1e-9f ? n / length : glm::vec3(0.0f, 0.0f, 1.0f);

            const int index = i * kStride + j;
            positions[index] = p;
            normals[index] = n;

            // Store (vertical fraction, cross-petal coordinate) so the
            // shader can use them even though the grid itself is (s, t).
            const float ang = glm::pi<float>() * t;
            texCoords[index] =
                glm::vec2(0.5f * (1.0f + std::cos(ang)), s * std::sin(ang));
        }
    }

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int face = 0; face < 2; ++face)
    {
        const float side = face == 0 ? 1.0f : -1.0f;
        for (int v = 0; v < kGridCount; ++v)
        {
            Vertex vertex;
            vertex.position = positions[v] + normals[v] * (side * kHalfThickness);
            vertex.texCoord = texCoords[v];
            mesh.vertices.push_back(vertex);
        }
    }

    // Front surface.
    for (int i = 0; i < kSegmentsS; ++i)
    {
        for (int j = 0; j < kSegmentsT; ++j)
        {
            const unsigned int a =
                base + static_cast<unsigned int>(i * kStride + j);
            const unsigned int b = a + 1;
            const unsigned int c = a + kStride;
            const unsigned int d = c + 1;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
            mesh.indices.push_back(c);
        }
    }

    // Back surface, wound the other way so its normals point away too.
    const unsigned int backBase = base + static_cast<unsigned int>(kGridCount);
    for (int i = 0; i < kSegmentsS; ++i)
    {
        for (int j = 0; j < kSegmentsT; ++j)
        {
            const unsigned int a =
                backBase + static_cast<unsigned int>(i * kStride + j);
            const unsigned int b = a + 1;
            const unsigned int c = a + kStride;
            const unsigned int d = c + 1;

            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(b);

            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
    }

    // Close the shell along the free rim (the s = 1 boundary).
    for (int j = 0; j < kSegmentsT; ++j)
    {
        const unsigned int f0 =
            base + static_cast<unsigned int>(kSegmentsS * kStride + j);
        const unsigned int f1 = f0 + 1;
        const unsigned int b0 = backBase + static_cast<unsigned int>(kSegmentsS * kStride + j);
        const unsigned int b1 = b0 + 1;

        mesh.indices.push_back(f0);
        mesh.indices.push_back(f1);
        mesh.indices.push_back(b0);

        mesh.indices.push_back(f1);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(b0);
    }
}

void AppendEllipsoid(MeshData& mesh, const glm::vec3& center,
                     const glm::vec3& radii);

void BuildRoseHead(MeshData& mesh)
{
    // A single continuous golden-angle spiral: every petal is slightly wider,
    // taller and more open than the previous one.  This avoids the visible
    // concentric rings a strict whorl layout produces.
    constexpr int kPetalCount = 48;

    for (int i = 0; i < kPetalCount; ++i)
    {
        const std::uint32_t index = static_cast<std::uint32_t>(i);
        const auto hash = [index](std::uint32_t salt)
        { return Hash01(index * 31u + salt); };

        const float u = static_cast<float>(i) /
                        static_cast<float>(kPetalCount - 1);

        const float radius = glm::mix(0.008f, 0.060f, std::pow(u, 0.90f)) *
                             (0.90f + 0.20f * hash(1));
        // The furled heart sits proud of the surrounding whorls, so the
        // bloom is domed rather than crowned with edge-on petal tips.
        const float yOffset = glm::mix(0.070f, -0.010f, u) +
                              (hash(2) - 0.5f) * 0.010f;
        // Every petal leans outward from the axis; the inner ones only
        // slightly, so they wrap the furled heart instead of standing up
        // edge-on and flashing as thin blades when seen from above.
        const float tilt = glm::mix(0.20f, 0.62f, std::pow(u, 1.10f)) +
                           (hash(3) - 0.5f) * 0.05f;
        // Inner petals are much smaller than outer ones; giving every petal
        // the same size stacks a dozen large sheets in the centre, which is
        // what made the heart of the bloom look like crumpled paper.
        const float height = glm::mix(0.175f, 0.290f, std::pow(u, 1.05f)) *
                             (0.98f + 0.04f * hash(4));
        const float width = glm::mix(0.085f, 0.255f, std::pow(u, 0.60f)) *
                            (0.95f + 0.10f * hash(5));
        // Cup depth must stay comparable to the petal's half width.  Larger
        // values fold the sheet into a narrow cone instead of a petal.
        const float cup = glm::mix(0.075f, 0.030f, std::pow(u, 0.55f)) *
                          (0.85f + 0.30f * hash(6));
        const float bend = glm::mix(-0.040f, 0.050f, std::pow(u, 1.30f)) +
                           (hash(7) - 0.5f) * 0.020f;
        const float twist = glm::mix(0.450f, 0.020f, std::pow(u, 0.80f)) *
                            (0.80f + 0.40f * hash(8));
        const float reflex = glm::mix(0.03f, 0.30f, std::pow(u, 0.85f)) *
                             (0.90f + 0.20f * hash(12));

        const float angle = static_cast<float>(i) * kGoldenAngle +
                            (hash(9) - 0.5f) * 0.26f;
        const float scale = 0.94f + 0.12f * hash(10);
        const float phase = hash(11) * glm::two_pi<float>();

        glm::mat4 model(1.0f);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(
            model, glm::vec3(0.0f, kHeadBaseY + yOffset, radius));
        model = glm::rotate(model, tilt, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale));

        AppendPetal(mesh, model, height, width, cup, bend, twist, phase,
                    reflex);
    }

}

// ---------------------------------------------------------------------------
// Stem, thorns, receptacle
// ---------------------------------------------------------------------------

glm::vec3 StemCenter(float u)
{
    const float y = glm::mix(kStemBottomY, kHeadBaseY, u);
    const float x = 0.050f * std::sin(u * glm::two_pi<float>()) * (1.0f - u);
    const float z = 0.032f * std::sin(u * 2.20f * glm::pi<float>()) *
                    (1.0f - u);
    return glm::vec3(x, y, z);
}

float StemRadius(float u)
{
    return 0.020f + 0.014f * std::pow(1.0f - u, 3.0f);
}

void AppendStem(MeshData& mesh)
{
    constexpr int kSegments = 20;
    constexpr int kRings = 72;

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int ring = 0; ring <= kRings; ++ring)
    {
        const float u = static_cast<float>(ring) / static_cast<float>(kRings);
        const glm::vec3 center = StemCenter(u);

        const float ds = 0.00075f;
        const glm::vec3 tangent = glm::normalize(
            StemCenter(std::min(1.0f, u + ds)) -
            StemCenter(std::max(0.0f, u - ds)));

        // Build a stable frame around the stem tangent.
        const glm::vec3 helper =
            std::abs(tangent.y) > 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                       : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 normalAxis = glm::normalize(glm::cross(tangent, helper));
        const glm::vec3 binormal = glm::cross(tangent, normalAxis);

        for (int segment = 0; segment < kSegments; ++segment)
        {
            const float angle = glm::two_pi<float>() *
                                static_cast<float>(segment) /
                                static_cast<float>(kSegments);
            const glm::vec3 direction = std::cos(angle) * normalAxis +
                                        std::sin(angle) * binormal;

            Vertex vertex;
            vertex.position = center + direction * StemRadius(u);
            vertex.texCoord =
                glm::vec2(u, static_cast<float>(segment) /
                                 static_cast<float>(kSegments));
            mesh.vertices.push_back(vertex);
        }
    }

    for (int ring = 0; ring < kRings; ++ring)
    {
        for (int segment = 0; segment < kSegments; ++segment)
        {
            const unsigned int next = (segment + 1) % kSegments;
            const unsigned int a =
                base + static_cast<unsigned int>(ring * kSegments + segment);
            const unsigned int b =
                base + static_cast<unsigned int>(ring * kSegments) + next;
            const unsigned int c = a + kSegments;
            const unsigned int d = base + static_cast<unsigned int>(
                                             (ring + 1) * kSegments) +
                                   next;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
            mesh.indices.push_back(c);
        }
    }
}

// A small cone used for thorns.  Its base is buried in the stem.
void AppendCone(MeshData& mesh, const glm::vec3& basePoint,
                const glm::vec3& direction, float length, float radius)
{
    constexpr int kSegments = 8;

    const glm::vec3 dir = glm::normalize(direction);
    const glm::vec3 helper =
        std::abs(dir.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f)
                               : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 normalAxis = glm::normalize(glm::cross(dir, helper));
    const glm::vec3 binormal = glm::cross(dir, normalAxis);

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int segment = 0; segment < kSegments; ++segment)
    {
        const float angle = glm::two_pi<float>() * static_cast<float>(segment) /
                            static_cast<float>(kSegments);

        Vertex vertex;
        vertex.position = basePoint +
                          (std::cos(angle) * normalAxis +
                           std::sin(angle) * binormal) *
                              radius;
        vertex.texCoord = glm::vec2(0.0f, static_cast<float>(segment) /
                                               static_cast<float>(kSegments));
        mesh.vertices.push_back(vertex);
    }

    Vertex tip;
    tip.position = basePoint + dir * length;
    tip.texCoord = glm::vec2(1.0f, 0.0f);
    mesh.vertices.push_back(tip);

    const unsigned int tipIndex =
        static_cast<unsigned int>(mesh.vertices.size()) - 1u;

    for (int segment = 0; segment < kSegments; ++segment)
    {
        const unsigned int a =
            base + static_cast<unsigned int>(segment);
        const unsigned int b =
            base + static_cast<unsigned int>((segment + 1) % kSegments);

        mesh.indices.push_back(a);
        mesh.indices.push_back(b);
        mesh.indices.push_back(tipIndex);
    }
}

// UV sphere scaled into an ellipsoid; used for the green receptacle under
// the flower head.
void AppendEllipsoid(MeshData& mesh, const glm::vec3& center,
                     const glm::vec3& radii)
{
    constexpr int kRings = 16;
    constexpr int kSectors = 24;

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int ring = 0; ring <= kRings; ++ring)
    {
        const float v = static_cast<float>(ring) / static_cast<float>(kRings);
        const float phi = v * glm::pi<float>();

        for (int sector = 0; sector < kSectors; ++sector)
        {
            const float u = static_cast<float>(sector) /
                            static_cast<float>(kSectors);
            const float theta = u * glm::two_pi<float>();

            const glm::vec3 unit(std::sin(phi) * std::cos(theta),
                                 std::cos(phi),
                                 std::sin(phi) * std::sin(theta));

            Vertex vertex;
            vertex.position = center + radii * unit;
            vertex.texCoord = glm::vec2(v, u);
            mesh.vertices.push_back(vertex);
        }
    }

    for (int ring = 0; ring < kRings; ++ring)
    {
        for (int sector = 0; sector < kSectors; ++sector)
        {
            const unsigned int next = (sector + 1) % kSectors;
            const unsigned int a =
                base + static_cast<unsigned int>(ring * kSectors + sector);
            const unsigned int b =
                base + static_cast<unsigned int>(ring * kSectors) + next;
            const unsigned int c = a + kSectors;
            const unsigned int d =
                base + static_cast<unsigned int>((ring + 1) * kSectors) + next;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
            mesh.indices.push_back(c);
        }
    }
}

void BuildGreenParts(MeshData& mesh)
{
    AppendStem(mesh);

    // A few thorns, spread around the visible part of the stem.
    const float thornPositions[] = {0.30f, 0.47f, 0.63f, 0.79f};
    for (std::uint32_t i = 0;
         i < static_cast<std::uint32_t>(std::size(thornPositions)); ++i)
    {
        const float u = thornPositions[i];
        const float angle = glm::two_pi<float>() * Hash01(i * 17u + 5u);

        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 center = StemCenter(u);
        const glm::vec3 base =
            center + radial * (StemRadius(u) * 0.80f);
        const glm::vec3 direction =
            glm::normalize(radial + glm::vec3(0.0f, 0.45f, 0.0f));

        AppendCone(mesh, base, direction, 0.034f, 0.009f);
    }

    // Green receptacle that the petals emerge from.
    AppendEllipsoid(mesh, glm::vec3(0.0f, kHeadBaseY - 0.018f, 0.0f),
                    glm::vec3(0.058f, 0.048f, 0.058f));
}

// ---------------------------------------------------------------------------
// Leaves and sepals
// ---------------------------------------------------------------------------

// A leaf grows along +Z from its base, with width along X.  `droop` bends the
// leaf downwards over its length and `fold` lifts the halves around the
// central vein.
glm::vec3 EvalLeaf(float s, float t, float length, float width, float droop,
                   float fold)
{
    float profile = std::pow(s, 0.55f) * std::pow(1.0f - s, 0.55f) / 0.466f;
    profile *= 1.0f + 0.045f * std::sin(s * 62.0f); // subtle serration

    const float x = width * profile * t;
    const float z = length * s;
    const float y = -droop * s * s + fold * t * t * (0.30f + 0.70f * s) +
                    0.010f * std::sin(s * 18.0f);

    return glm::vec3(x, y, z);
}

void AppendLeaf(MeshData& mesh, const glm::mat4& model, float length,
                float width, float droop, float fold)
{
    constexpr int kSegmentsS = 24;
    constexpr int kSegmentsT = 12;

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int i = 0; i <= kSegmentsS; ++i)
    {
        const float s = static_cast<float>(i) / static_cast<float>(kSegmentsS);

        for (int j = 0; j <= kSegmentsT; ++j)
        {
            const float t = -1.0f +
                            2.0f * static_cast<float>(j) /
                                static_cast<float>(kSegmentsT);

            const glm::vec3 local = EvalLeaf(s, t, length, width, droop, fold);

            Vertex vertex;
            vertex.position = glm::vec3(model * glm::vec4(local, 1.0f));
            vertex.texCoord = glm::vec2(s, t);
            mesh.vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < kSegmentsS; ++i)
    {
        for (int j = 0; j < kSegmentsT; ++j)
        {
            const unsigned int a =
                base + static_cast<unsigned int>(i * (kSegmentsT + 1) + j);
            const unsigned int b = a + 1;
            const unsigned int c = a + kSegmentsT + 1;
            const unsigned int d = c + 1;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
            mesh.indices.push_back(c);
        }
    }
}

// Sepals are shaped separately from leaves: a smooth lance that arcs down
// from the hip of the bloom, without the leaf's serration and midrib waves.
void AppendSepal(MeshData& mesh, const glm::mat4& model, float length,
                 float width, float droop)
{
    constexpr int kSegmentsS = 18;
    constexpr int kSegmentsT = 8;

    const unsigned int base = static_cast<unsigned int>(mesh.vertices.size());

    for (int i = 0; i <= kSegmentsS; ++i)
    {
        const float s = static_cast<float>(i) / static_cast<float>(kSegmentsS);
        const float profile = std::sin(glm::pi<float>() * std::pow(s, 0.80f));

        for (int j = 0; j <= kSegmentsT; ++j)
        {
            const float t = -1.0f +
                            2.0f * static_cast<float>(j) /
                                static_cast<float>(kSegmentsT);

            const glm::vec3 local(
                width * profile * t,
                -droop * std::pow(s, 1.6f) +
                    0.30f * width * t * t * (0.25f + 0.75f * s),
                length * s);

            Vertex vertex;
            vertex.position = glm::vec3(model * glm::vec4(local, 1.0f));
            vertex.texCoord = glm::vec2(s, t);
            mesh.vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < kSegmentsS; ++i)
    {
        for (int j = 0; j < kSegmentsT; ++j)
        {
            const unsigned int a =
                base + static_cast<unsigned int>(i * (kSegmentsT + 1) + j);
            const unsigned int b = a + 1;
            const unsigned int c = a + kSegmentsT + 1;
            const unsigned int d = c + 1;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
            mesh.indices.push_back(c);
        }
    }
}

struct LeafSpec
{
    float stemU;
    float angle;  // radians around the stem
    float tiltUp; // radians above horizontal
    float length;
    float width;
};

void BuildLeaves(MeshData& mesh)
{
    const std::vector<LeafSpec> leaves = {
        {0.56f, glm::radians(32.0f), 0.44f, 0.340f, 0.088f},
        {0.68f, glm::radians(158.0f), 0.52f, 0.300f, 0.080f},
        {0.80f, glm::radians(264.0f), 0.40f, 0.270f, 0.072f},
        {0.90f, glm::radians(78.0f), 0.32f, 0.205f, 0.056f},
    };

    for (const LeafSpec& leaf : leaves)
    {
        const glm::vec3 attach = StemCenter(leaf.stemU);

        glm::mat4 model(1.0f);
        model = glm::rotate(model, leaf.angle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(
            model, attach + glm::vec3(0.0f, 0.0f, StemRadius(leaf.stemU)));
        model = glm::rotate(model, -leaf.tiltUp, glm::vec3(1.0f, 0.0f, 0.0f));

        AppendLeaf(mesh, model, leaf.length, leaf.width, 0.11f, 0.028f);
    }

    // Five narrow sepals peeking out from under the flower head.
    constexpr int kSepalCount = 5;
    for (int i = 0; i < kSepalCount; ++i)
    {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) /
                                static_cast<float>(kSepalCount) +
                            0.28f;

        glm::mat4 model(1.0f);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(
            model, glm::vec3(0.0f, kHeadBaseY - 0.025f, 0.048f));
        model = glm::rotate(model, 0.95f, glm::vec3(1.0f, 0.0f, 0.0f));

        AppendSepal(mesh, model, 0.105f, 0.014f, 0.045f);
    }
}

// ---------------------------------------------------------------------------
// Palettes
// ---------------------------------------------------------------------------

struct Palette
{
    const char* name;
    glm::vec3 deep;
    glm::vec3 mid;
    glm::vec3 bright;
    glm::vec3 rim;
};

const std::array<Palette, 4> kPalettes = {{
    {"Crimson", glm::vec3(0.055f, 0.0010f, 0.0060f),
     glm::vec3(0.310f, 0.0030f, 0.0140f), glm::vec3(0.640f, 0.0140f, 0.0340f),
     glm::vec3(0.900f, 0.1000f, 0.1000f)},
    {"Blush", glm::vec3(0.22f, 0.040f, 0.080f),
     glm::vec3(0.72f, 0.22f, 0.30f), glm::vec3(1.00f, 0.55f, 0.62f),
     glm::vec3(1.00f, 0.76f, 0.78f)},
    {"Ivory", glm::vec3(0.30f, 0.22f, 0.10f),
     glm::vec3(0.85f, 0.70f, 0.45f), glm::vec3(1.00f, 0.94f, 0.80f),
     glm::vec3(1.00f, 0.98f, 0.90f)},
    {"Burgundy", glm::vec3(0.09f, 0.005f, 0.015f),
     glm::vec3(0.35f, 0.015f, 0.050f), glm::vec3(0.70f, 0.05f, 0.12f),
     glm::vec3(0.90f, 0.25f, 0.30f)},
}};
} // namespace

int main()
{
    try
    {
        Window window(1280, 720, "13_rose - Procedural Rose", true, 8);

        const std::string assetDir = ASSET_DIR;

        Shader roseShader(assetDir + "/shaders/rose.vert",
                          assetDir + "/shaders/rose.frag");
        Shader backdropShader(assetDir + "/shaders/backdrop.vert",
                              assetDir + "/shaders/backdrop.frag");
        Shader shadowShader(assetDir + "/shaders/shadow_depth.vert",
                            assetDir + "/shaders/shadow_depth.frag");
        ShadowMap shadowMap(2048);

        // The key light lives at the same direction as KEY_DIR in rose.frag.
        const glm::vec3 keyLightDir = glm::normalize(
            glm::vec3(-0.55f, 0.95f, 0.65f));
        const glm::vec3 shadowTarget(0.0f, kHeadBaseY + 0.10f, 0.0f);
        const glm::mat4 lightView = glm::lookAt(
            shadowTarget + keyLightDir * 3.5f, shadowTarget,
            glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightProjection =
            glm::ortho(-0.80f, 0.80f, -0.80f, 0.80f, 1.0f, 6.0f);
        const glm::mat4 lightSpace = lightProjection * lightView;

        // --- Generate the flower -------------------------------------------
        MeshData headData;
        BuildRoseHead(headData);
        ComputeNormals(headData);

        MeshData greenData;
        BuildGreenParts(greenData);
        ComputeNormals(greenData);

        MeshData leafData;
        BuildLeaves(leafData);
        ComputeNormals(leafData);

        // The furled centre of the bloom is kept separate so it can be shaded
        // as a bud rather than as a petal base (no deep base occlusion).
        MeshData budData;
        AppendEllipsoid(budData, glm::vec3(0.0f, kHeadBaseY + 0.062f, 0.0f),
                        glm::vec3(0.042f, 0.105f, 0.042f));
        ComputeNormals(budData);

        LOG_INFO("Rose geometry: head " << headData.vertices.size()
                                        << " verts / "
                                        << headData.indices.size() / 3
                                        << " tris, stems "
                                        << greenData.indices.size() / 3
                                        << " tris, leaves "
                                        << leafData.indices.size() / 3
                                        << " tris");

        Mesh roseHead(std::move(headData.vertices),
                      std::move(headData.indices));
        Mesh greenParts(std::move(greenData.vertices),
                        std::move(greenData.indices));
        Mesh leaves(std::move(leafData.vertices), std::move(leafData.indices));
        Mesh bud(std::move(budData.vertices), std::move(budData.indices));

        // Empty VAO for the full-screen background triangle (gl_VertexID).
        VertexArray backdropVertexArray;

        // --- Camera ---------------------------------------------------------
        Camera camera(glm::vec3(0.0f, 1.66f, 0.92f), -90.0f, -18.0f);
        camera.SetMovementSpeed(1.2f);
        camera.SetMouseSensitivity(0.08f);

        GLFWwindow* nativeWindow = window.GetNativeHandle();

        // The cursor is free by default.  Holding the left mouse button
        // captures it for as long as the button is down, and Tab toggles a
        // persistent capture for free look.  The demo never traps the pointer.
        bool cursorCaptured = false;
        bool cursorDisabled = false;

        int paletteIndex = 0;
        bool turntable = true;
        float spin = 0.0f;

        window.SetKeyCallback([&](int key, int scancode, int action, int mods)
        {
            (void)scancode;
            (void)mods;

            if (action != GLFW_PRESS)
            {
                return;
            }

            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(nativeWindow, GLFW_TRUE);
                break;
            case GLFW_KEY_SPACE:
                turntable = !turntable;
                LOG_INFO("Turntable: " << (turntable ? "on" : "off"));
                break;
            case GLFW_KEY_TAB:
                cursorCaptured = !cursorCaptured;
                LOG_INFO("Mouse look: "
                         << (cursorCaptured ? "captured (Tab to release)"
                                            : "free (hold LMB to look)"));
                break;
            case GLFW_KEY_1:
            case GLFW_KEY_2:
            case GLFW_KEY_3:
            case GLFW_KEY_4: {
                paletteIndex = key - GLFW_KEY_1;
                LOG_INFO("Palette: "
                         << kPalettes[static_cast<std::size_t>(paletteIndex)]
                                .name);
                break;
            }
            default:
                break;
            }
        });

        bool firstMouse = true;
        double lastX = 0.0;
        double lastY = 0.0;

        window.SetCursorPosCallback([&](double xPos, double yPos)
        {
            // Only steer the camera while the pointer is captured, either by
            // Tab or by holding the left button.  Otherwise the free cursor
            // just moves around the desktop.
            const bool looking =
                cursorCaptured ||
                glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT) ==
                    GLFW_PRESS;

            if (!looking)
            {
                firstMouse = true;
                return;
            }

            if (firstMouse)
            {
                lastX = xPos;
                lastY = yPos;
                firstMouse = false;
                return;
            }

            const float offsetX = static_cast<float>(xPos - lastX);
            const float offsetY = static_cast<float>(lastY - yPos);
            lastX = xPos;
            lastY = yPos;

            camera.ProcessMouse(offsetX, offsetY);
        });

        window.SetScrollCallback([&](double xOffset, double yOffset)
        {
            (void)xOffset;
            camera.ProcessMouseScroll(static_cast<float>(yOffset));
        });

        LOG_INFO("13_rose controls: WASD move, Q/E down/up, hold LMB or press "
                 "TAB for mouse look, wheel zoom, SPACE turntable, 1-4 palette "
                 "(Crimson/Blush/Ivory/Burgundy), ESC quit");

        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glDepthFunc(GL_LESS));

        float lastFrame = static_cast<float>(glfwGetTime());

        while (!window.ShouldClose())
        {
            const float currentFrame = static_cast<float>(glfwGetTime());
            const float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            window.PollEvents();

            // Capture the pointer only while asked to: Tab for persistent free
            // look, or holding the left button for a momentary drag.
            const bool wantCapture =
                cursorCaptured ||
                glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT) ==
                    GLFW_PRESS;
            if (wantCapture != cursorDisabled)
            {
                cursorDisabled = wantCapture;
                glfwSetInputMode(nativeWindow, GLFW_CURSOR,
                                 cursorDisabled ? GLFW_CURSOR_DISABLED
                                                : GLFW_CURSOR_NORMAL);
                firstMouse = true;
            }

            if (glfwGetKey(nativeWindow, GLFW_KEY_W) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Forward);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_S) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Backward);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_A) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Left);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_D) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Right);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_E) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Up);
            }
            if (glfwGetKey(nativeWindow, GLFW_KEY_Q) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(deltaTime, CameraMovement::Down);
            }

            if (turntable)
            {
                spin += deltaTime * 0.30f;
            }

            const float time = static_cast<float>(glfwGetTime());

            // --- Scene transform -------------------------------------------
            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = camera.GetProjectionMatrix(
                static_cast<float>(window.GetWidth()) /
                static_cast<float>(window.GetHeight()));

            glm::mat4 model(1.0f);
            model = glm::rotate(model, spin, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, std::sin(time * 0.90f) * 0.008f,
                                glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, std::sin(time * 0.60f + 1.3f) * 0.006f,
                                glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::mat3 normalMatrix =
                glm::mat3(glm::transpose(glm::inverse(model)));

            // --- Shadow pass ----------------------------------------------
            // Every surface writes its depth once (nothing is culled), so
            // overlapping petals occlude each other and the bloom gains real
            // depth instead of reading as flat paper.
            shadowMap.BeginWrite(lightSpace);
            shadowShader.Bind();
            shadowShader.SetUniform("u_LightSpaceMatrix", lightSpace);
            shadowShader.SetUniform("u_Model", model);

            GLCall(glDisable(GL_CULL_FACE));

            greenParts.Draw(shadowShader);
            shadowShader.Bind();
            shadowShader.SetUniform("u_LightSpaceMatrix", lightSpace);
            shadowShader.SetUniform("u_Model", model);
            leaves.Draw(shadowShader);
            shadowShader.Bind();
            shadowShader.SetUniform("u_LightSpaceMatrix", lightSpace);
            shadowShader.SetUniform("u_Model", model);
            roseHead.Draw(shadowShader);
            shadowShader.Bind();
            shadowShader.SetUniform("u_LightSpaceMatrix", lightSpace);
            shadowShader.SetUniform("u_Model", model);
            bud.Draw(shadowShader);

            shadowShader.Unbind();
            shadowMap.EndWrite(window.GetWidth(), window.GetHeight());

            Renderer::Clear(0.0f, 0.0f, 0.0f, 1.0f);

            // --- Background gradient --------------------------------------
            GLCall(glDisable(GL_DEPTH_TEST));
            GLCall(glDepthMask(GL_FALSE));

            backdropShader.Bind();
            backdropShader.SetUniform(
                "u_Resolution",
                glm::vec2(static_cast<float>(window.GetWidth()),
                          static_cast<float>(window.GetHeight())));
            backdropShader.SetUniform("u_Time", time);

            backdropVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 3));
            backdropVertexArray.Unbind();

            backdropShader.Unbind();

            GLCall(glDepthMask(GL_TRUE));
            GLCall(glEnable(GL_DEPTH_TEST));
            GLCall(glClear(GL_DEPTH_BUFFER_BIT));

            // --- Scene -----------------------------------------------------
            const Palette& palette =
                kPalettes[static_cast<std::size_t>(paletteIndex)];

            shadowMap.BindForReading(4);

            roseShader.Bind();
            roseShader.SetUniform("uModel", model);
            roseShader.SetUniform("uView", view);
            roseShader.SetUniform("uProjection", projection);
            roseShader.SetUniform("uNormalMatrix", normalMatrix);
            roseShader.SetUniform("uViewPos", camera.GetPosition());
            roseShader.SetUniform("uTime", time);
            roseShader.SetUniform("uHeadCenter",
                                  glm::vec3(0.0f, kHeadBaseY, 0.0f));
            roseShader.SetUniform("uPetalDeep", palette.deep);
            roseShader.SetUniform("uPetalMid", palette.mid);
            roseShader.SetUniform("uPetalBright", palette.bright);
            roseShader.SetUniform("uPetalRim", palette.rim);
            roseShader.SetUniform("uShadowMap", 4);
            roseShader.SetUniform("uLightSpaceMatrix", lightSpace);
            roseShader.SetUniform("uShadowBias", 0.0010f);
            roseShader.SetUniform("uUseShadows", 1);

            // Mesh::Draw unbinds the program, so rebind before each part and
            // before setting the per-part material mode.
            roseShader.Bind();
            roseShader.SetUniform("uMode", 1);
            greenParts.Draw(roseShader);

            roseShader.Bind();
            roseShader.SetUniform("uMode", 2);
            leaves.Draw(roseShader);

            roseShader.Bind();
            roseShader.SetUniform("uMode", 0);
            roseHead.Draw(roseShader);

            roseShader.Bind();
            roseShader.SetUniform("uMode", 3);
            bud.Draw(roseShader);

            window.SwapBuffers();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(e.what());
        return 1;
    }

    return 0;
}
