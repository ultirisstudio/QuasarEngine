#pragma once

#include <quasar/renderer/Framebuffer.h>

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace TestHelpers
{
    using Color = std::array<int, 4>;

    inline bool IsNear(const Color& color, const Color& expected, int tolerance = 1)
    {
        return std::ranges::equal(color, expected, [tolerance](int value, int other) { return std::abs(value - other) <= tolerance; });
    }

    inline Color ReadWindowPixel(int x, int y)
    {
        std::array<std::uint8_t, 4> pixel{};
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return {pixel[0], pixel[1], pixel[2], pixel[3]};
    }

    inline Color ReadColor(Quasar::Framebuffer& framebuffer, int x, int y)
    {
        framebuffer.Bind();
        const Color color = ReadWindowPixel(x, static_cast<int>(framebuffer.GetSpecification().Height) - 1 - y);
        framebuffer.Unbind();
        return color;
    }
}
