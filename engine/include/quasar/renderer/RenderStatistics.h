#pragma once

/// \file
/// \brief The counts of the work that a frame sends to the graphics card, such as its draws and its binds.
/// \author Kurogenshi
/// \date 2026-11-26

#include <cstdint>

namespace Quasar
{
    /// @brief What the engine sent to the graphics card during one frame.
    ///
    /// The graphics context remembers what is bound and set, and does not send again what is already in place: the
    /// binds and the changes counted here are the ones that were sent. The draws of the ImGui windows are not counted:
    /// the ImGui backend sends them itself.
    struct RenderStatistics
    {
        std::uint32_t Draws = 0;              ///< The draw calls.
        std::uint64_t Triangles = 0;          ///< The triangles of the draw calls.
        std::uint32_t PipelineBinds = 0;      ///< Pipeline::Bind() calls, which send only the states that change.
        std::uint32_t FramebufferBinds = 0;   ///< The binds of a framebuffer or of the window.
        std::uint32_t TextureBinds = 0;       ///< The binds of a texture or of a framebuffer attachment to a slot.
        std::uint32_t UniformBufferBinds = 0; ///< The binds of a uniform buffer.
        std::uint32_t StateChanges = 0;       ///< The other changes: shader, vertex array, viewport, depth, culling and blending.
        std::uint32_t UniformChanges = 0;     ///< The values set on a shader, such as Shader::SetFloat().
        std::uint32_t Uploads = 0;            ///< The writes of data into a buffer or a texture.
        std::uint64_t UploadedBytes = 0;      ///< The bytes of these writes.

        /// @brief Compares all the counts.
        bool operator==(const RenderStatistics&) const = default;
    };
}
