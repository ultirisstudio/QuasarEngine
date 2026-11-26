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

    inline std::uint8_t ReadRed8(GLuint texture, int x, int y)
    {
        GLint alignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        std::uint8_t red = 0;
        glGetTextureSubImage(texture, 0, x, y, 0, 1, 1, 1, GL_RED, GL_UNSIGNED_BYTE, sizeof(red), &red);
        glPixelStorei(GL_PACK_ALIGNMENT, alignment);
        return red;
    }
}
