#pragma once

/// \file
/// \brief A light of a scene that shines from a point in every direction, such as a bulb.
/// \author Kurogenshi
/// \date 2026-11-20

#include <glm/vec3.hpp>

namespace Quasar
{
    /// @brief A light of a scene that shines from a point in every direction, such as a bulb.
    ///
    /// The light shines from the position of its entity in the world. See PointLight for how it fades with the
    /// distance, and Renderer::MaxPointLights for the number of point lights that a scene can use.
    struct PointLightComponent
    {
        glm::vec3 Color{1.0f};  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f; ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;    ///< The distance at which the light ends.
    };
}
