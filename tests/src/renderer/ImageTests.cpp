#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Image.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <vector>

using Quasar::Image;

namespace
{
    // 2x2: red, half transparent green, then transparent blue and (10, 20, 30, 40)
    constexpr std::array<unsigned char, 83> RgbaPng{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00,
        0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xB6, 0x0D, 0x24, 0x00, 0x00, 0x00, 0x1A, 0x49, 0x44, 0x41, 0x54, 0x78,
        0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0xF0, 0x9F, 0xE1, 0x3F, 0x43, 0x03, 0x03, 0x03, 0xC3, 0x7F, 0x06, 0x2E, 0x11, 0x39, 0x0D, 0x00,
        0x36, 0xA9, 0x04, 0xE1, 0xF4, 0x7A, 0x5A, 0xDB, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    // 1x2 without alpha: (1, 2, 3) above (250, 251, 252)
    constexpr std::array<unsigned char, 73> RgbPng{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x08,
        0x02, 0x00, 0x00, 0x00, 0x16, 0xE3, 0x21, 0x70, 0x00, 0x00, 0x00, 0x10, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0x60, 0x64, 0x62, 0x66, 0xF8, 0xF5,
        0xFB, 0x0F, 0x00, 0x06, 0x0A, 0x02, 0xF8, 0xA4, 0x29, 0x76, 0x42, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    // 3x1 grayscale: 0, 100 and 255
    constexpr std::array<unsigned char, 69> GrayPng{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x8B, 0x4B, 0x68, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0x60, 0x48,
        0xF9, 0x0F, 0x00, 0x01, 0xCB, 0x01, 0x64, 0x6F, 0xF3, 0x01, 0xAE, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    template <std::size_t Size>
    std::span<const std::byte> AsBytes(const std::array<unsigned char, Size>& data)
    {
        return std::as_bytes(std::span(data));
    }

    std::vector<std::byte> ToBytes(std::initializer_list<int> values)
    {
        std::vector<std::byte> bytes;
        for (const int value : values)
        {
            bytes.push_back(static_cast<std::byte>(value));
        }
        return bytes;
    }
}

TEST_CASE("An image is decoded into 4 bytes per pixel, from the top row")
{
    const TestHelpers::ScopedLog log;

    const std::optional<Image> image = Image::FromMemory(AsBytes(RgbaPng));

    REQUIRE(image.has_value());
    CHECK(image->Width == 2);
    CHECK(image->Height == 2);
    CHECK(image->Pixels == ToBytes({255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 0, 10, 20, 30, 40}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("An image without alpha is opaque, and a grayscale image has the same red, green and blue")
{
    const TestHelpers::ScopedLog log;

    const std::optional<Image> rgb = Image::FromMemory(AsBytes(RgbPng));
    const std::optional<Image> gray = Image::FromMemory(AsBytes(GrayPng));

    REQUIRE(rgb.has_value());
    CHECK(rgb->Width == 1);
    CHECK(rgb->Height == 2);
    CHECK(rgb->Pixels == ToBytes({1, 2, 3, 255, 250, 251, 252, 255}));
    REQUIRE(gray.has_value());
    CHECK(gray->Width == 3);
    CHECK(gray->Height == 1);
    CHECK(gray->Pixels == ToBytes({0, 0, 0, 255, 100, 100, 100, 255, 255, 255, 255, 255}));
}

TEST_CASE("An image is decoded from a file")
{
    const TestHelpers::ScopedLog log;
    const std::span<const std::byte> bytes = AsBytes(RgbaPng);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const TestHelpers::TemporaryFile file("Sprite.png", std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()));

    const std::optional<Image> image = Image::FromFile(file.GetPath());

    REQUIRE(image.has_value());
    CHECK(image->Pixels == Image::FromMemory(bytes)->Pixels);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("Data that is not an image, a truncated image or a missing file gives no image")
{
    const std::vector<std::byte> text = ToBytes({'Q', 'u', 'a', 's', 'a', 'r'});
    const TestHelpers::TemporaryFolder folder;

    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Image::FromMemory(text).has_value());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Image::FromMemory(AsBytes(RgbaPng).first(40)).has_value());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Image::FromMemory({}).has_value());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(Image::FromFile(folder.GetPath() / "Missing.png").has_value());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}
