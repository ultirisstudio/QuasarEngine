#pragma once

/// \file
/// \brief The settings of the shadows of a directional light, and the camera that draws them.
/// \author Kurogenshi
/// \date 2026-11-20

#include <quasar/renderer/Camera.h>

#include <glm/vec3.hpp>

#include <cstdint>

namespace Quasar
{
    /// @brief Settings of the shadows of the directional light.
    ///
    /// The shadows are drawn into a square map, seen from the light, that covers the part of the view of the camera up
    /// to Distance. A smaller distance gives sharper shadows. Without bias, a surface can shadow itself in stripes,
    /// called shadow acne; too much bias detaches the shadows from the objects.
    struct ShadowSettings
    {
        bool Enabled = true;             ///< Whether the directional light casts shadows.
        std::uint32_t Resolution = 2048; ///< The width and the height of the map, in pixels.
        float Distance = 40.0f;          ///< The distance from the camera up to which the shadows are drawn.
        float DepthBias = 0.0005f;       ///< The depth, from 0 to 1 across the map, removed from each surface before the test.
        float NormalBias = 1.5f;         ///< How far each surface is moved along its normal before the test, in pixels of the map.
    };

    /// @brief The camera that draws the shadow map of a directional light, see ComputeShadowCamera().
    struct ShadowCamera
    {
        Camera View;                  ///< The orthographic camera, looking along the light.
        std::uint32_t Resolution = 1; ///< The width and the height of the map, ShadowSettings::Resolution or at least 1.
        float TexelSize = 0.0f;       ///< The size of one pixel of the map, in world units.
    };

    /// @brief Computes the camera that draws the shadow map of a directional light for the view of a camera.
    ///
    /// The part of the view up to ShadowSettings::Distance is wrapped in a sphere, whose size does not change when the
    /// camera turns, and the map covers that sphere. Its center moves only by whole pixels of the map. Both keep the
    /// shadows from shimmering when the camera moves.
    /// @param camera The camera of the view.
    /// @param direction The direction in which the light shines, which does not need a length of 1. A zero direction
    /// shines down.
    /// @param settings The resolution and the distance of the shadows.
    ShadowCamera ComputeShadowCamera(const Camera& camera, const glm::vec3& direction, const ShadowSettings& settings);
}
