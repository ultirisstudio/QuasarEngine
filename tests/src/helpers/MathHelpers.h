#pragma once

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace TestHelpers
{
    inline constexpr float DefaultTolerance = 1e-5f;

    inline bool IsNear(float value, float expected, float tolerance = DefaultTolerance)
    {
        return std::abs(value - expected) < tolerance;
    }

    inline bool IsNear(const glm::vec3& value, const glm::vec3& expected, float tolerance = DefaultTolerance)
    {
        return glm::distance(value, expected) < tolerance;
    }

    inline bool IsNear(const glm::mat4& value, const glm::mat4& expected, float tolerance = DefaultTolerance)
    {
        for (glm::length_t column = 0; column < 4; ++column)
        {
            if (!(glm::distance(value[column], expected[column]) < tolerance))
            {
                return false;
            }
        }
        return true;
    }
}
