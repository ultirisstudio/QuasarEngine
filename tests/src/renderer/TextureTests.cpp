#include "helpers/LogHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Texture.h>

#include <doctest/doctest.h>

#include <array>

using Quasar::TextureSpecification;

TEST_CASE("A texture needs a width and a height")
{
    {
        const TestHelpers::ScopedLog log;
        CHECK(Quasar::Texture2D::IsValid({.Width = 1, .Height = 1}));
        CHECK(Quasar::Texture2D::IsValid({.Width = 4096, .Height = 2}));
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }

    for (const TextureSpecification& specification :
         std::array<TextureSpecification, 3>{{{.Width = 0, .Height = 1}, {.Width = 1, .Height = 0}, {.Width = 0, .Height = 0}}})
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Quasar::Texture2D::IsValid(specification));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
