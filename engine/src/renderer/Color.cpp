#include <quasar/renderer/Color.h>

#include <cmath>

namespace Quasar
{
    float SrgbToLinear(float value)
    {
        if (value <= 0.04045f)
        {
            return value > 0.0f ? value / 12.92f : 0.0f;
        }
        return std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    float LinearToSrgb(float value)
    {
        if (value <= 0.0031308f)
        {
            return value > 0.0f ? value * 12.92f : 0.0f;
        }
        return (1.055f * std::pow(value, 1.0f / 2.4f)) - 0.055f;
    }

    glm::vec3 SrgbToLinear(const glm::vec3& color)
    {
        return {SrgbToLinear(color.x), SrgbToLinear(color.y), SrgbToLinear(color.z)};
    }

    glm::vec4 SrgbToLinear(const glm::vec4& color)
    {
        return {SrgbToLinear(glm::vec3(color)), color.w};
    }

    glm::vec3 LinearToSrgb(const glm::vec3& color)
    {
        return {LinearToSrgb(color.x), LinearToSrgb(color.y), LinearToSrgb(color.z)};
    }

    glm::vec4 LinearToSrgb(const glm::vec4& color)
    {
        return {LinearToSrgb(glm::vec3(color)), color.w};
    }
}
