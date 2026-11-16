#include "engine/Renderer/ObjLoader.h"

#include <glm/glm.hpp>

#include <sstream>
#include <string>

#include "TestFramework.h"

namespace
{
ObjLoader::Data ParseText(const std::string& text)
{
    std::istringstream stream(text);
    return ObjLoader::Parse(stream, "<test>");
}

const std::string kTriangle =
    "v 0 0 0\n"
    "v 1 0 0\n"
    "v 0 1 0\n"
    "f 1 2 3\n";
} // namespace

TEST(SingleTriangleProducesThreeVertices)
{
    const ObjLoader::Data data = ParseText(kTriangle);

    CHECK_EQ(data.vertices.size(), 3u);
    CHECK_EQ(data.indices.size(), 3u);

    CHECK_NEAR(data.vertices[0].position.x, 0.0f, 1e-6f);
    CHECK_NEAR(data.vertices[1].position.x, 1.0f, 1e-6f);
    CHECK_NEAR(data.vertices[2].position.y, 1.0f, 1e-6f);

    CHECK_EQ(data.indices[0], 0u);
    CHECK_EQ(data.indices[1], 1u);
    CHECK_EQ(data.indices[2], 2u);
}

TEST(MissingNormalsAreGeneratedAndNormalized)
{
    const ObjLoader::Data data = ParseText(kTriangle);

    for (const Vertex& vertex : data.vertices)
    {
        CHECK_NEAR(glm::length(vertex.normal), 1.0f, 1e-5f);
        CHECK_NEAR(vertex.normal.x, 0.0f, 1e-5f);
        CHECK_NEAR(vertex.normal.y, 0.0f, 1e-5f);
        CHECK_NEAR(vertex.normal.z, 1.0f, 1e-5f);
    }
}

TEST(ProvidedNormalsAreNormalized)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vn 0 0 5\n"
        "f 1//1 2//1 3//1\n");

    for (const Vertex& vertex : data.vertices)
    {
        CHECK_NEAR(glm::length(vertex.normal), 1.0f, 1e-5f);
        CHECK_NEAR(vertex.normal.z, 1.0f, 1e-5f);
    }
}

TEST(FullVertexIndexFormCarriesTexCoords)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/2/1 3/3/1\n");

    CHECK_NEAR(data.vertices[1].texCoord.x, 1.0f, 1e-6f);
    CHECK_NEAR(data.vertices[2].texCoord.y, 1.0f, 1e-6f);
}

TEST(TextureOnlyIndexFormIsAccepted)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0.25 0.5\n"
        "f 1/1 2/1 3/1\n");

    CHECK_NEAR(data.vertices[0].texCoord.x, 0.25f, 1e-6f);
    CHECK_NEAR(data.vertices[0].texCoord.y, 0.5f, 1e-6f);
}

TEST(NegativeIndicesAreResolved)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f -3 -2 -1\n");

    CHECK_EQ(data.vertices.size(), 3u);
    CHECK_NEAR(data.vertices[2].position.y, 1.0f, 1e-6f);
}

TEST(QuadIsTriangulatedAsAFan)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "f 1 2 3 4\n");

    CHECK_EQ(data.vertices.size(), 6u);
    CHECK_EQ(data.indices.size(), 6u);
}

TEST(SharedPositionGetsAveragedNormal)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n"
        "f 1 3 4\n");

    // The shared position appears in both triangles, so every copy must
    // receive the same accumulated, normalized normal.
    CHECK_NEAR(data.vertices[0].normal.x, data.vertices[3].normal.x, 1e-6f);
    CHECK_NEAR(data.vertices[0].normal.y, data.vertices[3].normal.y, 1e-6f);
    CHECK_NEAR(data.vertices[0].normal.z, data.vertices[3].normal.z, 1e-6f);
    CHECK_NEAR(glm::length(data.vertices[0].normal), 1.0f, 1e-5f);
}

TEST(DegenerateTriangleFallsBackToUpNormal)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 2 0 0\n"
        "f 1 2 3\n");

    CHECK_NEAR(data.vertices[0].normal.y, 1.0f, 1e-5f);
}

TEST(CommentsAndUnknownTagsAreIgnored)
{
    const ObjLoader::Data data = ParseText(
        "# a comment\n"
        "mtllib scene.mtl\n"
        "o object\n"
        "g group\n"
        "usemtl material\n"
        "s 1\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n");

    CHECK_EQ(data.vertices.size(), 3u);
}

TEST(WindowsLineEndingsAreHandled)
{
    const ObjLoader::Data data = ParseText(
        "v 0 0 0\r\n"
        "v 1 0 0\r\n"
        "v 0 1 0\r\n"
        "f 1 2 3\r\n");

    CHECK_EQ(data.vertices.size(), 3u);
}

TEST(EmptyInputIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText(""), "no usable geometry");
}

TEST(PositionsWithoutFacesAreRejected)
{
    CHECK_THROWS_MESSAGE(ParseText("v 0 0 0\n"), "no usable geometry");
}

TEST(FaceWithTooFewCornersIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText("v 0 0 0\nv 1 0 0\nf 1 2\n"), "fewer than 3");
}

TEST(OutOfRangeIndexIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText(kTriangle + "f 1 2 4\n"), "out of range");
}

TEST(NonNumericIndexIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText(kTriangle + "f 1 2 x\n"), "invalid index");
}

TEST(MalformedVertexIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText("v 1 2\n"), "malformed vertex");
}

TEST(MalformedTexCoordIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText("vt 1\n"), "malformed texcoord");
}

TEST(MalformedNormalIsRejected)
{
    CHECK_THROWS_MESSAGE(ParseText("vn 1 2\n"), "malformed normal");
}

TEST(MissingFileIsRejected)
{
    CHECK_THROWS_MESSAGE(ObjLoader::Load("/nonexistent/class-opengl/missing.obj"),
                         "Failed to open OBJ model");
}

int main()
{
    return ::testing::RunAll();
}
