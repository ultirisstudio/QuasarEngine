#pragma once

/// \file
/// \brief A light of a scene that shines from a point in a cone, such as a torch.
/// \author Kurogenshi
/// \date 2026-11-20

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace Quasar
{
    /// @brief A light of a scene that shines from a point in a cone, such as a torch.
    ///
    /// The light shines from the position of its entity in the world, along its forward direction, the -Z axis turned by
    /// its rotation. See SpotLight for how it fades, and MaxSpotLights for the number of spot lights that a
    /// scene can use.
    struct SpotLightComponent
    {
        glm::vec3 Color{1.0f};                  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f;                 ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;                    ///< The distance at which the light ends.
        float InnerAngle = glm::radians(20.0f); ///< The angle of the full light, from the axis, in radians.
        float OuterAngle = glm::radians(30.0f); ///< The angle where the light ends, from the axis, in radians, below 180 degrees.
    };
}
