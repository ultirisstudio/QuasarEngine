#pragma once

/// \file
/// \brief Vertex arrays: the vertex buffers and the index buffer that a draw reads.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/VertexLayout.h>

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Quasar
{
    /// @brief The vertex buffers and the index buffer of a mesh, which GraphicsContext::Draw() and
    /// GraphicsContext::DrawIndexed() read.
    ///
    /// A vertex array is created empty with GraphicsContext::CreateVertexArray() and owns the buffers given to it. A
    /// vertex array without vertex buffers can still be drawn, by a vertex shader that only uses `gl_VertexID`, for
    /// example to draw a triangle that covers the framebuffer.
    class VertexArray
    {
    public:
        virtual ~VertexArray() = default;

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&&) = delete;
        VertexArray& operator=(VertexArray&&) = delete;

        /// @brief Adds a vertex buffer, whose attributes go to the locations of its layout.
        ///
        /// A buffer that is null or whose layout uses a location that an earlier buffer already uses is refused with
        /// an engine error, and destroyed.
        void AddVertexBuffer(Scope<VertexBuffer> buffer);

        /// @brief Sets the index buffer, which replaces and destroys the previous one. A null buffer is refused with an
        /// engine error.
        void SetIndexBuffer(Scope<IndexBuffer> buffer);

        /// @brief Returns the number of vertex buffers.
        std::size_t GetVertexBufferCount() const { return m_VertexBuffers.size(); }

        /// @brief Returns a vertex buffer, to change its content.
        /// @param index The number of the buffer, in the order in which they were added, which must exist.
        VertexBuffer& GetVertexBuffer(std::size_t index) { return *m_VertexBuffers[index]; }

        /// @brief Returns a vertex buffer.
        /// @param index The number of the buffer, in the order in which they were added, which must exist.
        const VertexBuffer& GetVertexBuffer(std::size_t index) const { return *m_VertexBuffers[index]; }

        /// @brief Returns the index buffer, or nullptr if there is none.
        IndexBuffer* GetIndexBuffer() { return m_IndexBuffer.get(); }

        /// @brief Returns the index buffer, or nullptr if there is none.
        const IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

    protected:
        VertexArray() = default;

        /// @brief Makes the draws read a vertex buffer, with the number it has in the vertex array.
        virtual void AttachVertexBuffer(const VertexBuffer& buffer, std::uint32_t index) = 0;

        /// @brief Makes the indexed draws read an index buffer.
        virtual void AttachIndexBuffer(const IndexBuffer& buffer) = 0;

    private:
        std::vector<Scope<VertexBuffer>> m_VertexBuffers;
        Scope<IndexBuffer> m_IndexBuffer;
        std::bitset<VertexLayout::MaxLocations> m_UsedLocations;
    };
}
