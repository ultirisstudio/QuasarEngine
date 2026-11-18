#include "helpers/LogHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/VertexLayout.h>

#include <doctest/doctest.h>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

using Quasar::VertexAttributeType;
using Quasar::VertexLayout;

namespace
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::ivec2 Ids;
        glm::mat4 Transform;
        float Weight;
        glm::mat3 Basis;
        glm::vec2 TexCoord;
        glm::ivec4 Bones;
        glm::vec4 Color;
        int Index;
        glm::ivec3 Cell;
    };
}

TEST_CASE("A vertex layout places its attributes one after the other, like the members of a struct")
{
    const TestHelpers::ScopedLog log;
    const VertexLayout layout{
        {0, VertexAttributeType::Vec3}, {1, VertexAttributeType::IVec2},  {2, VertexAttributeType::Mat4},   {6, VertexAttributeType::Float},
        {7, VertexAttributeType::Mat3}, {10, VertexAttributeType::Vec2},  {11, VertexAttributeType::IVec4}, {12, VertexAttributeType::Vec4},
        {13, VertexAttributeType::Int}, {14, VertexAttributeType::IVec3},
    };
    constexpr std::array<std::size_t, 10> offsets{
        offsetof(Vertex, Position), offsetof(Vertex, Ids),   offsetof(Vertex, Transform), offsetof(Vertex, Weight), offsetof(Vertex, Basis),
        offsetof(Vertex, TexCoord), offsetof(Vertex, Bones), offsetof(Vertex, Color),     offsetof(Vertex, Index),  offsetof(Vertex, Cell),
    };

    REQUIRE(layout.GetAttributes().size() == offsets.size());
    for (std::size_t attribute = 0; attribute < offsets.size(); ++attribute)
    {
        CAPTURE(attribute);
        CHECK(layout.GetOffset(attribute) == offsets.at(attribute));
    }
    CHECK(layout.GetStride() == sizeof(Vertex));
    CHECK(layout.IsValid());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A matrix attribute takes one location per column")
{
    CHECK(Quasar::GetVertexAttributeLocationCount(VertexAttributeType::Mat3) == 3);
    CHECK(Quasar::GetVertexAttributeLocationCount(VertexAttributeType::Mat4) == 4);
    CHECK(Quasar::GetVertexAttributeLocationCount(VertexAttributeType::Vec4) == 1);
    CHECK(Quasar::GetVertexAttributeLocationCount(VertexAttributeType::IVec4) == 1);

    const TestHelpers::ScopedLog log;
    CHECK(VertexLayout{{0, VertexAttributeType::Mat4}, {4, VertexAttributeType::Mat3}, {7, VertexAttributeType::Vec4}}.IsValid());
    CHECK(VertexLayout{{12, VertexAttributeType::Mat4}}.IsValid());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A vertex layout without attributes, with a location used twice or with a location out of range is refused")
{
    const std::array<VertexLayout, 7> layouts{
        VertexLayout{},
        VertexLayout{{1, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec2}},
        VertexLayout{{0, VertexAttributeType::Mat4}, {3, VertexAttributeType::Float}},
        VertexLayout{{2, VertexAttributeType::Float}, {0, VertexAttributeType::Mat3}},
        VertexLayout{{VertexLayout::MaxLocations, VertexAttributeType::Float}},
        VertexLayout{{100, VertexAttributeType::Float}},
        VertexLayout{{13, VertexAttributeType::Mat4}},
    };

    for (std::size_t index = 0; index < layouts.size(); ++index)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(index);
        CHECK_FALSE(layouts.at(index).IsValid());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
