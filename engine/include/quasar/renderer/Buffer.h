#pragma once

/// \file
/// \brief Buffers: memory of the graphics card that holds the vertices, the indices and the uniform blocks of the draws.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/renderer/VertexLayout.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace Quasar
{
    /// @brief The vertices of a mesh, stored as described by their VertexLayout.
    ///
    /// A vertex buffer is created with GraphicsContext::CreateVertexBuffer() from its first content, which also gives
    /// its size. The size never changes: SetData() replaces a part of the content, and a larger content needs a new
    /// buffer. A vertex buffer is drawn once it is added to a VertexArray.
    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() = default;

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&&) = delete;
        VertexBuffer& operator=(VertexBuffer&&) = delete;

        /// @brief Returns true if a vertex buffer can be created with this size and layout, whatever the graphics API.
        ///
        /// The size must not be 0 and must hold a whole number of vertices, and the layout must be valid. The reason a
        /// buffer is refused is logged as an engine error.
        static bool IsValid(std::size_t size, const VertexLayout& layout);

        /// @brief Replaces a part of the content, for example with `std::as_bytes(std::span(vertices))`.
        /// @param data The new bytes.
        /// @param offset The position of the first byte to replace, in bytes.
        ///
        /// Data that does not fit in the buffer is refused with an engine error, and the content is not changed.
        void SetData(std::span<const std::byte> data, std::size_t offset = 0);

        /// @brief Returns the size of the buffer, in bytes.
        std::size_t GetSize() const { return m_Size; }

        /// @brief Returns the number of vertices that the buffer holds.
        std::uint32_t GetVertexCount() const { return static_cast<std::uint32_t>(m_Size / m_Layout.GetStride()); }

        /// @brief Returns the layout of the vertices.
        const VertexLayout& GetLayout() const { return m_Layout; }

    protected:
        /// @brief Keeps the size and the layout, which must be valid. See IsValid().
        VertexBuffer(std::size_t size, VertexLayout layout);

        /// @brief Copies bytes into the buffer. The bytes fit in the buffer.
        virtual void Write(std::span<const std::byte> data, std::size_t offset) = 0;

    private:
        std::size_t m_Size = 0;
        VertexLayout m_Layout;
    };

    /// @brief The indices of the vertices of each triangle of a mesh, as 32-bit unsigned integers.
    ///
    /// An index buffer is created with GraphicsContext::CreateIndexBuffer() from its indices, which also give its size.
    /// It is drawn with GraphicsContext::DrawIndexed() once it is set in a VertexArray.
    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;
        IndexBuffer(IndexBuffer&&) = delete;
        IndexBuffer& operator=(IndexBuffer&&) = delete;

        /// @brief Returns true if an index buffer of this number of indices can be created, whatever the graphics API.
        /// It must not be 0 or more than 2^31 - 1. The reason a buffer is refused is logged as an engine error.
        static bool IsValid(std::size_t count);

        /// @brief Replaces some of the indices.
        /// @param indices The new indices.
        /// @param first The number of the first index to replace.
        ///
        /// Indices that do not fit in the buffer are refused with an engine error, and the content is not changed.
        void SetData(std::span<const std::uint32_t> indices, std::size_t first = 0);

        /// @brief Returns the number of indices, which DrawIndexed() draws.
        std::uint32_t GetCount() const { return m_Count; }

    protected:
        /// @brief Keeps the number of indices, which must be valid. See IsValid().
        explicit IndexBuffer(std::uint32_t count);

        /// @brief Copies indices into the buffer. The indices fit in the buffer.
        virtual void Write(std::span<const std::uint32_t> indices, std::size_t first) = 0;

    private:
        std::uint32_t m_Count = 0;
    };

    /// @brief The values of a uniform block of the shaders, such as the camera or a material.
    ///
    /// A uniform buffer is created with GraphicsContext::CreateUniformBuffer() with its size and the binding of the
    /// block that it fills, the N of `layout(std140, binding = N) uniform Camera { ... };`. Bind() makes the next draws
    /// read the block from this buffer. Set() writes a whole C++ struct, whose members must follow the std140 rules of
    /// GLSL: a `vec3` or a `vec4` starts at a multiple of 16 bytes, so a struct of `glm::vec4` and `glm::mat4` members
    /// is the simplest.
    class UniformBuffer
    {
    public:
        virtual ~UniformBuffer() = default;

        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;
        UniformBuffer(UniformBuffer&&) = delete;
        UniformBuffer& operator=(UniformBuffer&&) = delete;

        /// @brief Returns true if a uniform buffer of this size can be created, whatever the graphics API. It must not be
        /// 0. The reason a buffer is refused is logged as an engine error.
        static bool IsValid(std::size_t size);

        /// @brief Replaces a part of the content.
        /// @param data The new bytes.
        /// @param offset The position of the first byte to replace, in bytes.
        ///
        /// Data that does not fit in the buffer is refused with an engine error, and the content is not changed.
        void SetData(std::span<const std::byte> data, std::size_t offset = 0);

        /// @brief Replaces the start of the content with a value, usually a struct with the members of the block.
        template <typename T>
        void Set(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "A uniform buffer is filled with the bytes of the value");
            SetData(std::as_bytes(std::span<const T, 1>(&value, 1)));
        }

        /// @brief Makes the next draws read the uniform block of the binding of this buffer from it.
        virtual void Bind() = 0;

        /// @brief Returns the size of the buffer, in bytes.
        std::size_t GetSize() const { return m_Size; }

        /// @brief Returns the binding of the uniform block that the buffer fills.
        std::uint32_t GetBinding() const { return m_Binding; }

    protected:
        /// @brief Keeps the size, which must be valid, and the binding. See IsValid().
        UniformBuffer(std::size_t size, std::uint32_t binding);

        /// @brief Copies bytes into the buffer. The bytes fit in the buffer.
        virtual void Write(std::span<const std::byte> data, std::size_t offset) = 0;

    private:
        std::size_t m_Size = 0;
        std::uint32_t m_Binding = 0;
    };
}
