#include "helpers/LogHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/UniformBlockLayout.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

using Quasar::UniformBlockLayout;
using Quasar::UniformType;

TEST_CASE("A uniform block layout places its members with the offsets of std140")
{
    const UniformBlockLayout layout{
        {"a", UniformType::Float}, {"b", UniformType::Vec3}, {"c", UniformType::Vec2}, {"d", UniformType::Float},
        {"e", UniformType::Vec4},  {"f", UniformType::Int},  {"g", UniformType::Mat4}, {"h", UniformType::Vec2},
    };
    constexpr std::array<std::uint32_t, 8> offsets{0, 16, 32, 40, 48, 64, 80, 144};

    REQUIRE(layout.GetMembers().size() == offsets.size());
    for (std::size_t member = 0; member < offsets.size(); ++member)
    {
        CAPTURE(member);
        CHECK(layout.GetOffset(member) == offsets.at(member));
    }
    CHECK(layout.GetSize() == 160);
}

TEST_CASE("A scalar after a vec3 fills the end of its 16 bytes, and the size is a multiple of 16")
{
    const UniformBlockLayout layout{{"Direction", UniformType::Vec3}, {"Intensity", UniformType::Float}, {"Count", UniformType::Int}};

    CHECK(layout.GetOffset(0) == 0);
    CHECK(layout.GetOffset(1) == 12);
    CHECK(layout.GetOffset(2) == 16);
    CHECK(layout.GetSize() == 32);
    CHECK(UniformBlockLayout{}.GetSize() == 0);
}

TEST_CASE("A member of a uniform block layout is found by its name")
{
    const UniformBlockLayout layout{{"Color", UniformType::Vec4}, {"Roughness", UniformType::Float}};

    CHECK(layout.Find("Color") == std::optional<std::size_t>(0));
    CHECK(layout.Find("Roughness") == std::optional<std::size_t>(1));
    CHECK_FALSE(layout.Find("Metallic").has_value());
    CHECK_FALSE(layout.Find("color").has_value());
}

TEST_CASE("A uniform block layout with a member without a name or two members with the same name is refused")
{
    {
        const TestHelpers::ScopedLog log;
        CHECK(UniformBlockLayout{{"Color", UniformType::Vec4}, {"Scale", UniformType::Float}}.IsValid());
        CHECK(UniformBlockLayout{}.IsValid());
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    for (const UniformBlockLayout& layout : std::array<UniformBlockLayout, 2>{
             UniformBlockLayout{{"Color", UniformType::Vec4}, {"", UniformType::Float}},
             UniformBlockLayout{{"Color", UniformType::Vec4}, {"Scale", UniformType::Float}, {"Color", UniformType::Vec3}},
         })
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(layout.IsValid());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
