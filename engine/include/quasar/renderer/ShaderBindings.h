#pragma once

/// \file
/// \brief The bindings of the blocks and the texture slots that the shaders share with the Renderer.
/// \author Kurogenshi
/// \date 2026-11-20

#include <cstdint>

/// @brief The bindings of the blocks and the texture slots that the shaders share with the Renderer.
///
/// The Renderer fills the uniform blocks Camera and Lights, the storage block Objects and the slots AmbientOcclusionMap
/// and ShadowMap, so a Material cannot use them. See Renderer for the content of the blocks.
namespace Quasar::ShaderBindings
{
    inline constexpr std::uint32_t Camera = 0;               ///< The `Camera` block, filled by the Renderer for each scene.
    inline constexpr std::uint32_t Material = 1;             ///< The block of the values of a Material, by default.
    inline constexpr std::uint32_t Objects = 2;              ///< The `Objects` storage block, filled by the Renderer for each frame.
    inline constexpr std::uint32_t Lights = 3;               ///< The `Lights` block, filled by the Renderer for each scene.
    inline constexpr std::uint32_t AmbientOcclusionMap = 14; ///< The texture slot of the `u_AmbientOcclusionMap`, bound by the Renderer.
    inline constexpr std::uint32_t ShadowMap = 15;           ///< The texture slot of the `u_ShadowMap`, bound by the Renderer.

    /// @brief Returns true if the Renderer fills the uniform block of this binding. The storage block Objects has its
    /// own bindings, apart from the uniform blocks.
    constexpr bool IsFilledByRenderer(std::uint32_t binding)
    {
        return binding == Camera || binding == Lights;
    }

    /// @brief Returns true if the Renderer binds a texture to this slot.
    constexpr bool IsBoundByRenderer(std::uint32_t slot)
    {
        return slot == AmbientOcclusionMap || slot == ShadowMap;
    }
}
