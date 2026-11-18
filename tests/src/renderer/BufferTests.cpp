#include "helpers/LogHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/VertexLayout.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

using Quasar::VertexAttributeType;
using Quasar::VertexLayout;

TEST_CASE("A vertex buffer needs a valid layout and a whole number of vertices")
{
    const VertexLayout layout{{0, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec2}};
    {
        const TestHelpers::ScopedLog log;
        CHECK(Quasar::VertexBuffer::IsValid(20, layout));
        CHECK(Quasar::VertexBuffer::IsValid(60, layout));
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }

    struct Refused
    {
        std::size_t Size = 0;
        VertexLayout Layout;
    };
    const std::array<Refused, 4> refused{{
        {.Size = 0, .Layout = layout},
        {.Size = 30, .Layout = layout},
        {.Size = 20, .Layout = VertexLayout{}},
        {.Size = 12, .Layout = VertexLayout{{0, VertexAttributeType::Vec2}, {0, VertexAttributeType::Float}}},
    }};
    for (const Refused& buffer : refused)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(buffer.Size);
        CHECK_FALSE(Quasar::VertexBuffer::IsValid(buffer.Size, buffer.Layout));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}

TEST_CASE("Index buffers and uniform buffers cannot be empty")
{
    {
        const TestHelpers::ScopedLog log;
        CHECK(Quasar::IndexBuffer::IsValid(1));
        CHECK(Quasar::IndexBuffer::IsValid(std::numeric_limits<std::int32_t>::max()));
        CHECK(Quasar::UniformBuffer::IsValid(1));
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Quasar::IndexBuffer::IsValid(0));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Quasar::IndexBuffer::IsValid(static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) + 1));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Quasar::UniformBuffer::IsValid(0));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
