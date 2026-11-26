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
    /// Each bind is counted, even when it binds again what is already bound. The draws of the ImGui windows are not
    /// counted: the ImGui backend sends them itself.
    struct RenderStatistics
    {
        std::uint32_t Draws = 0;              ///< The draw calls.
        std::uint64_t Triangles = 0;          ///< The triangles of the draw calls.
        std::uint32_t PipelineBinds = 0;      ///< Pipeline::Bind() calls, which each set the shader and the fixed states.
        std::uint32_t FramebufferBinds = 0;   ///< The binds of a framebuffer or of the window, Framebuffer::Unbind() included.
        std::uint32_t TextureBinds = 0;       ///< The binds of a texture or of a framebuffer attachment to a slot.
        std::uint32_t UniformBufferBinds = 0; ///< UniformBuffer::Bind() calls.
        std::uint32_t UniformChanges = 0;     ///< The values set on a shader, such as Shader::SetFloat().
        std::uint32_t Uploads = 0;            ///< The writes of data into a buffer or a texture.
        std::uint64_t UploadedBytes = 0;      ///< The bytes of these writes.

        /// @brief Compares all the counts.
        bool operator==(const RenderStatistics&) const = default;
    };
}
