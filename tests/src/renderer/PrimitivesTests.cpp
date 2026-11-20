#include "helpers/LogHelpers.h"
#include "helpers/MathHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Primitives.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>

using Quasar::MeshData;
using Quasar::Vertex;
using TestHelpers::IsNear;

namespace
{
    std::array<const Vertex*, 3> GetTriangle(const MeshData& data, std::size_t triangle)
    {
        return {&data.Vertices.at(data.Indices.at(triangle * 3)), &data.Vertices.at(data.Indices.at(triangle * 3 + 1)),
                &data.Vertices.at(data.Indices.at(triangle * 3 + 2))};
    }

    glm::vec3 GetFaceNormal(const std::array<const Vertex*, 3>& triangle)
    {
        return glm::cross(triangle[1]->Position - triangle[0]->Position, triangle[2]->Position - triangle[0]->Position);
    }

    // The volume enclosed by the triangles, which is only right for a closed surface whose triangles all face outward
    float GetVolume(const MeshData& data)
    {
        float volume = 0.0f;
        for (std::size_t triangle = 0; triangle < data.Indices.size() / 3; ++triangle)
        {
            const std::array<const Vertex*, 3> vertices = GetTriangle(data, triangle);
            volume += glm::dot(vertices[0]->Position, glm::cross(vertices[1]->Position, vertices[2]->Position)) / 6.0f;
        }
        return volume;
    }

    bool FacesItsNormals(const MeshData& data)
    {
        if (data.Indices.empty() || data.Indices.size() % 3 != 0)
        {
            return false;
        }
        for (std::size_t triangle = 0; triangle < data.Indices.size() / 3; ++triangle)
        {
            const std::array<const Vertex*, 3> vertices = GetTriangle(data, triangle);
            const glm::vec3 face = GetFaceNormal(vertices);
            if (!(glm::length(face) > 1e-6f))
            {
                return false;
            }
            for (const Vertex* vertex : vertices)
            {
                if (!(glm::dot(face, vertex->Normal) > 0.0f) || std::abs(glm::length(vertex->Normal) - 1.0f) > 1e-5f)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool HasTexCoordsInImage(const MeshData& data)
    {
        return std::ranges::all_of(data.Vertices,
                                   [](const Vertex& vertex) {
                                       return vertex.TexCoord.x >= 0.0f && vertex.TexCoord.x <= 1.0f && vertex.TexCoord.y >= 0.0f && vertex.TexCoord.y <= 1.0f;
                                   });
    }

    bool IsUpright(const MeshData& data)
    {
        for (std::size_t triangle = 0; triangle < data.Indices.size() / 3; ++triangle)
        {
            const std::array<const Vertex*, 3> vertices = GetTriangle(data, triangle);
            if (std::abs(vertices[0]->Normal.y) > 0.5f)
            {
                continue;
            }
            for (const Vertex* first : vertices)
            {
                for (const Vertex* second : vertices)
                {
                    if (first->TexCoord.y < second->TexCoord.y && !(first->Position.y > second->Position.y))
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
}

TEST_CASE("The standard vertex has the layout of its struct")
{
    const Quasar::VertexLayout layout = Vertex::GetLayout();

    REQUIRE(layout.GetAttributes().size() == 3);
    CHECK(layout.GetStride() == sizeof(Vertex));
    CHECK(layout.GetOffset(0) == offsetof(Vertex, Position));
    CHECK(layout.GetOffset(1) == offsetof(Vertex, Normal));
    CHECK(layout.GetOffset(2) == offsetof(Vertex, TexCoord));
    CHECK(layout.GetAttributes()[0].Location == 0);
    CHECK(layout.GetAttributes()[1].Location == 1);
    CHECK(layout.GetAttributes()[2].Location == 2);
}

TEST_CASE("A cube is closed, faces outward, and has its own normal and the whole image on each face")
{
    const MeshData cube = Quasar::Primitives::CreateCube(2.0f);

    CHECK(cube.Vertices.size() == 24);
    CHECK(cube.Indices.size() == 36);
    CHECK(FacesItsNormals(cube));
    CHECK(std::abs(GetVolume(cube) - 8.0f) < 1e-4f);
    CHECK(HasTexCoordsInImage(cube));
    CHECK(IsUpright(cube));
    for (const glm::vec3 direction : {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)})
    {
        CAPTURE(direction.x + 2.0f * direction.y + 3.0f * direction.z);
        CHECK(std::ranges::count_if(cube.Vertices, [direction](const Vertex& vertex) { return vertex.Normal == direction; }) == 4);
    }
    for (const Vertex& vertex : cube.Vertices)
    {
        CHECK(IsNear(glm::length(vertex.Position), std::sqrt(3.0f)));
        CHECK(IsNear(glm::dot(vertex.Position, vertex.Normal), 1.0f));
    }
}

TEST_CASE("A cube has the top of the image of its top face toward -Z, and of its bottom face toward +Z")
{
    const MeshData cube = Quasar::Primitives::CreateCube();

    for (const Vertex& vertex : cube.Vertices)
    {
        if (vertex.Normal.y > 0.5f)
        {
            CHECK((vertex.TexCoord.y == 0.0f) == (vertex.Position.z < 0.0f));
            CHECK((vertex.TexCoord.x == 0.0f) == (vertex.Position.x < 0.0f));
        }
        else if (vertex.Normal.y < -0.5f)
        {
            CHECK((vertex.TexCoord.y == 0.0f) == (vertex.Position.z > 0.0f));
            CHECK((vertex.TexCoord.x == 0.0f) == (vertex.Position.x < 0.0f));
        }
    }
}

TEST_CASE("A quad faces +Z, with the image upright and not mirrored")
{
    const MeshData quad = Quasar::Primitives::CreateQuad(3.0f);

    CHECK(quad.Vertices.size() == 4);
    CHECK(quad.Indices.size() == 6);
    CHECK(FacesItsNormals(quad));
    CHECK(IsUpright(quad));
    for (const Vertex& vertex : quad.Vertices)
    {
        CHECK(vertex.Normal == glm::vec3(0.0f, 0.0f, 1.0f));
        CHECK(vertex.Position.z == 0.0f);
        CHECK(std::abs(vertex.Position.x) == 1.5f);
        CHECK(std::abs(vertex.Position.y) == 1.5f);
        CHECK((vertex.TexCoord.x == 0.0f) == (vertex.Position.x < 0.0f));
        CHECK((vertex.TexCoord.y == 0.0f) == (vertex.Position.y > 0.0f));
    }
}

TEST_CASE("A sphere is closed, faces outward, and has its vertices on its radius")
{
    const MeshData sphere = Quasar::Primitives::CreateSphere(2.0f, 64, 32);

    CHECK(sphere.Vertices.size() == 65 * 33);
    CHECK(sphere.Indices.size() == 6 * 64 * 31);
    CHECK(FacesItsNormals(sphere));
    CHECK(std::abs(GetVolume(sphere) - 4.0f / 3.0f * glm::pi<float>() * 8.0f) < 0.02f * 33.5f);
    CHECK(HasTexCoordsInImage(sphere));
    CHECK(IsUpright(sphere));
    for (const Vertex& vertex : sphere.Vertices)
    {
        CHECK(IsNear(glm::length(vertex.Position), 2.0f));
        CHECK(glm::distance(vertex.Position / 2.0f, vertex.Normal) < 1e-5f);
    }
}

TEST_CASE("A sphere has v = 0 at its top pole, u growing toward the east, and the u of each pole vertex in the middle of its triangle")
{
    const MeshData sphere = Quasar::Primitives::CreateSphere(1.0f, 8, 4);

    for (const Vertex& vertex : sphere.Vertices)
    {
        CHECK((vertex.TexCoord.y == 0.0f) == (vertex.Position.y > 0.999f));
        CHECK((vertex.TexCoord.y == 1.0f) == (vertex.Position.y < -0.999f));
        if (std::abs(vertex.Position.y) < 0.999f && vertex.Position.z > 0.5f && std::abs(vertex.Position.x) > 1e-3f)
        {
            CHECK((vertex.Position.x > 0.0f) == (vertex.TexCoord.x < 0.25f));
        }
    }
    for (std::size_t triangle = 0; triangle < sphere.Indices.size() / 3; ++triangle)
    {
        const std::array<const Vertex*, 3> vertices = GetTriangle(sphere, triangle);
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            if (std::abs(vertices.at(corner)->Position.y) > 0.999f)
            {
                const float middle = (vertices.at((corner + 1) % 3)->TexCoord.x + vertices.at((corner + 2) % 3)->TexCoord.x) / 2.0f;
                CHECK(IsNear(vertices.at(corner)->TexCoord.x, middle));
            }
        }
    }
    const MeshData smallest = Quasar::Primitives::CreateSphere(1.0f, 3, 2);
    CHECK(FacesItsNormals(smallest));
    CHECK(GetVolume(smallest) > 0.0f);
}

TEST_CASE("A shape of a size that is not above 0, or a sphere with too few segments or rings, gives no data")
{
    const std::array<MeshData (*)(), 9> builders{
        [] { return Quasar::Primitives::CreateCube(0.0f); },
        [] { return Quasar::Primitives::CreateCube(-1.0f); },
        [] { return Quasar::Primitives::CreateCube(std::numeric_limits<float>::quiet_NaN()); },
        [] { return Quasar::Primitives::CreateQuad(0.0f); },
        [] { return Quasar::Primitives::CreateQuad(std::numeric_limits<float>::infinity()); },
        [] { return Quasar::Primitives::CreateSphere(0.0f); },
        [] { return Quasar::Primitives::CreateSphere(1.0f, 2, 16); },
        [] { return Quasar::Primitives::CreateSphere(1.0f, 32, 1); },
        [] { return Quasar::Primitives::CreateSphere(std::numeric_limits<float>::quiet_NaN()); },
    };

    for (std::size_t index = 0; index < builders.size(); ++index)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(index);
        const MeshData data = builders.at(index)();
        CHECK(data.Vertices.empty());
        CHECK(data.Indices.empty());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
