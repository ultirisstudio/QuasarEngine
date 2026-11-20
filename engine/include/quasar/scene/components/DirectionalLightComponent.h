#pragma once

/// \file
/// \brief A light of a scene that comes from the same direction everywhere, such as the sun.
/// \author Kurogenshi
/// \date 2026-11-19

#include <glm/vec3.hpp>

namespace Quasar
{
    /// @brief A light of a scene that comes from the same direction everywhere, such as the sun.
    ///
    /// The light goes in the forward direction of its entity in the world, the -Z axis turned by its rotation. See
    /// Entity::GetWorldTransform(). The SceneRenderer lights the scene with the first one it finds, and a scene without
    /// one has no directional light and no ambient light. See DirectionalLight.
    struct DirectionalLightComponent
    {
        glm::vec3 Color{1.0f};  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f; ///< The factor of the color.
        float Ambient = 0.1f;   ///< The part of the light that reaches every surface, even the ones that face away.
    };
}
