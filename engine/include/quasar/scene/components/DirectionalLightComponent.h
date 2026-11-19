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
    /// The light goes in the forward direction of the TransformComponent of its entity, the -Z axis turned by its
    /// rotation. The SceneRenderer lights the scene with the first one it finds. See DirectionalLight.
    struct DirectionalLightComponent
    {
        glm::vec3 Color{1.0f};  ///< The color of the light.
        float Intensity = 1.0f; ///< The factor of the color.
        float Ambient = 0.1f;   ///< The part of the light that reaches every surface, even the ones that face away.
    };
}
