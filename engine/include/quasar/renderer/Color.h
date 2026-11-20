#pragma once

/// \file
/// \brief Conversions of colors between sRGB, the space of images and color pickers, and linear values, the space of
/// lighting.
/// \author Kurogenshi
/// \date 2026-11-20

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Quasar
{
    /// @brief Converts a channel from sRGB, as in an image or a color picker, to a linear value, in which light adds up.
    /// The exact sRGB curve is used. Values below 0 give 0.
    float SrgbToLinear(float value);

    /// @brief Converts a linear channel to sRGB, as the screen expects it. The exact sRGB curve is used. Values below 0
    /// give 0, and values above 1 keep growing on the same curve.
    float LinearToSrgb(float value);

    /// @brief Converts the red, green and blue of a color from sRGB to linear values. See SrgbToLinear(float).
    glm::vec3 SrgbToLinear(const glm::vec3& color);

    /// @brief Converts the red, green and blue of a color from sRGB to linear values, and keeps its alpha, which is
    /// already linear. See SrgbToLinear(float).
    glm::vec4 SrgbToLinear(const glm::vec4& color);

    /// @brief Converts the red, green and blue of a color from linear values to sRGB. See LinearToSrgb(float).
    glm::vec3 LinearToSrgb(const glm::vec3& color);

    /// @brief Converts the red, green and blue of a color from linear values to sRGB, and keeps its alpha. See
    /// LinearToSrgb(float).
    glm::vec4 LinearToSrgb(const glm::vec4& color);
}
