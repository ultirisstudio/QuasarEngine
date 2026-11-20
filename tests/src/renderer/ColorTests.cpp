#include <quasar/renderer/Color.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace
{
    bool IsNear(float value, float expected)
    {
        return std::abs(value - expected) < 1e-5f;
    }
}

TEST_CASE("A channel goes from sRGB to linear and back on the exact sRGB curve")
{
    CHECK(Quasar::SrgbToLinear(0.0f) == 0.0f);
    CHECK(IsNear(Quasar::SrgbToLinear(1.0f), 1.0f));
    CHECK(IsNear(Quasar::SrgbToLinear(0.5f), 0.2140411f));
    CHECK(IsNear(Quasar::SrgbToLinear(0.04f), 0.04f / 12.92f));
    CHECK(Quasar::SrgbToLinear(-0.5f) == 0.0f);

    CHECK(Quasar::LinearToSrgb(0.0f) == 0.0f);
    CHECK(IsNear(Quasar::LinearToSrgb(1.0f), 1.0f));
    CHECK(IsNear(Quasar::LinearToSrgb(0.5f), 0.7353570f));
    CHECK(IsNear(Quasar::LinearToSrgb(0.003f), 0.003f * 12.92f));
    CHECK(IsNear(Quasar::LinearToSrgb(2.0f), 1.3532560f));
    CHECK(Quasar::LinearToSrgb(-0.5f) == 0.0f);

    for (const float value : {0.01f, 0.04045f, 0.2f, 0.5f, 0.9f})
    {
        CAPTURE(value);
        CHECK(IsNear(Quasar::LinearToSrgb(Quasar::SrgbToLinear(value)), value));
    }
}

TEST_CASE("A color goes from sRGB to linear and back on red, green and blue, and keeps its alpha")
{
    const glm::vec4 linear = Quasar::SrgbToLinear(glm::vec4(0.0f, 0.5f, 1.0f, 0.5f));
    CHECK(glm::distance(linear, glm::vec4(0.0f, 0.2140411f, 1.0f, 0.5f)) < 1e-5f);

    const glm::vec4 srgb = Quasar::LinearToSrgb(glm::vec4(0.0f, 0.5f, 1.0f, 0.25f));
    CHECK(glm::distance(srgb, glm::vec4(0.0f, 0.7353570f, 1.0f, 0.25f)) < 1e-5f);

    CHECK(glm::distance(Quasar::SrgbToLinear(glm::vec3(0.5f, 1.0f, 0.0f)), glm::vec3(0.2140411f, 1.0f, 0.0f)) < 1e-5f);
    CHECK(glm::distance(Quasar::LinearToSrgb(glm::vec3(0.5f, 1.0f, 0.0f)), glm::vec3(0.7353570f, 1.0f, 0.0f)) < 1e-5f);
}
