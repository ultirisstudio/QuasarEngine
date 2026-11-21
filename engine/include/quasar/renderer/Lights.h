#pragma once

/// \file
/// \brief The lights of a scene, as the Renderer draws them: a directional light with its shadows, point lights and spot
/// lights.
/// \author Kurogenshi
/// \date 2026-11-20

#include <quasar/renderer/Framebuffer.h>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace Quasar
{
    inline constexpr std::size_t MaxPointLights = 32; ///< The most point lights of a scene, the size of the `u_PointLights` array of the shaders.
    inline constexpr std::size_t MaxSpotLights = 32;  ///< The most spot lights of a scene, the size of the `u_SpotLights` array of the shaders.

    /// @brief A light that comes from the same direction everywhere, such as the sun.
    struct DirectionalLight
    {
        /// The direction in which the light goes, which does not need a length of 1. A zero direction goes down.
        glm::vec3 Direction{-0.4f, -1.0f, -0.6f};
        glm::vec3 Color{1.0f};  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f; ///< The factor of the color.
        float Ambient = 0.1f;   ///< The part of the light that reaches every surface, even the ones that face away.
    };

    /// @brief A light that shines from a point in every direction, such as a bulb.
    ///
    /// It lights a surface by its intensity divided by the square of the distance, as a real light does, and fades
    /// smoothly to nothing at its range, so that it lights only the surfaces near it.
    struct PointLight
    {
        glm::vec3 Position{0.0f}; ///< The position, in the world.
        glm::vec3 Color{1.0f};    ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f;   ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;      ///< The distance at which the light ends.
    };

    /// @brief A light that shines from a point in a cone, such as a torch, and fades with the distance like a PointLight.
    ///
    /// The light is full inside the inner angle, and fades smoothly to nothing at the outer angle. Both angles go from the
    /// axis of the cone to its side, and an inner angle wider than the outer one gives a sharp edge at the outer one.
    struct SpotLight
    {
        glm::vec3 Position{0.0f};               ///< The position, in the world.
        glm::vec3 Direction{0.0f, 0.0f, -1.0f}; ///< The axis of the cone, which does not need a length of 1.
        glm::vec3 Color{1.0f};                  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f;                 ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;                    ///< The distance at which the light ends.
        float InnerAngle = glm::radians(20.0f); ///< The angle of the full light, in radians.
        float OuterAngle = glm::radians(30.0f); ///< The angle where the light ends, in radians, below 180 degrees.
    };

    /// @brief The shadows of the directional light: a map of the depth of the scene seen from the light, which the
    /// shaders compare with the depth of each surface.
    struct DirectionalShadow
    {
        /// The framebuffer of the map, with a depth attachment of format FramebufferFormat::Depth32F, or nullptr for no
        /// shadows. It must stay alive until Renderer::EndScene().
        const Framebuffer* Map = nullptr;

        glm::mat4 ViewProjection{1.0f}; ///< The matrix of the camera of the light, which projects the world onto the map.
        float DepthBias = 0.0f;         ///< The depth, from 0 to 1 in the map, removed from each surface before the test.
        float NormalBias = 0.0f;        ///< The distance along its normal by which each surface is moved before the test.
    };

    /// @brief The ambient occlusion of a scene: the share of the ambient light that reaches each pixel of the target.
    struct SceneAmbientOcclusion
    {
        /// The framebuffer that holds the share in red in its first color attachment, of the size of the target, such as
        /// AmbientOcclusion::GetResult(), or nullptr for no ambient occlusion. It must stay alive until Renderer::EndScene().
        const Framebuffer* Map = nullptr;

        /// The share of the ambient occlusion also applied to the direct lights, from 0 to 1. See
        /// AmbientOcclusionSettings::DirectLightingStrength.
        float DirectLightingStrength = 0.0f;
    };

    /// @brief The lights of a scene: at most one directional light, with its shadows, and point lights and spot lights.
    struct SceneLights
    {
        std::optional<DirectionalLight> Directional = DirectionalLight{}; ///< The directional light, if the scene has one.
        DirectionalShadow Shadow{};                                       ///< The shadows of the directional light.
        std::vector<PointLight> Points{};                                 ///< The point lights.
        std::vector<SpotLight> Spots{};                                   ///< The spot lights.
        SceneAmbientOcclusion AmbientOcclusion{};                         ///< How much ambient light reaches each pixel.
    };
}
