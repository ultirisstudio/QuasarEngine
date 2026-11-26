#pragma once

/// \file
/// \brief Buffers: memory of the graphics card that holds the vertices, the indices and the uniform blocks of the draws.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/renderer/VertexLayout.h>

#include <cstddef>
#include <cstdint>
#include <optional>
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

    /// @brief A uniform block written again at each frame in many parts of one size, such as the object of each draw.
    ///
    /// It is created with GraphicsContext::CreateFrameUniformBuffer() with the size of a part and the binding of the
    /// block that it fills. Add() writes a part and gives its index, and Bind() makes the next draws read the block from
    /// a part. The parts of a frame are kept until GraphicsContext::EndFrame(), then the next frame starts again from
    /// the part 0. Each frame writes into its own memory, which the graphics card no longer reads, so writing never
    /// waits for it. The buffer grows when a frame adds more parts than it holds.
    class FrameUniformBuffer
    {
    public:
        virtual ~FrameUniformBuffer() = default;

        FrameUniformBuffer(const FrameUniformBuffer&) = delete;
        FrameUniformBuffer& operator=(const FrameUniformBuffer&) = delete;
        FrameUniformBuffer(FrameUniformBuffer&&) = delete;
        FrameUniformBuffer& operator=(FrameUniformBuffer&&) = delete;

        /// @brief Returns true if parts of this size can be created, whatever the graphics API. It must not be 0. The
        /// reason a buffer is refused is logged as an engine error.
        static bool IsValid(std::size_t partSize);

        /// @brief Writes a part for the current frame.
        /// @param data The bytes of the part, of the size of a part.
        /// @return The index of the part, or nullopt if the data has another size or if the buffer cannot grow. The
        /// reason is logged as an engine error.
        std::optional<std::uint32_t> AddData(std::span<const std::byte> data);

        /// @brief Writes a part from a value, usually a struct with the members of the block. See AddData().
        template <typename T>
        std::optional<std::uint32_t> Add(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "A frame uniform buffer is filled with the bytes of the value");
            return AddData(std::as_bytes(std::span<const T, 1>(&value, 1)));
        }

        /// @brief Makes the next draws read the uniform block of the binding of this buffer from a part of the current
        /// frame. A part that the current frame did not add is refused with an engine error.
        void Bind(std::uint32_t part);

        /// @brief Returns the number of parts that the current frame added.
        virtual std::uint32_t GetPartCount() const = 0;

        /// @brief Returns the size of a part, in bytes.
        std::size_t GetPartSize() const { return m_PartSize; }

        /// @brief Returns the binding of the uniform block that the buffer fills.
        std::uint32_t GetBinding() const { return m_Binding; }

    protected:
        /// @brief Keeps the size of a part, which must be valid, and the binding. See IsValid().
        FrameUniformBuffer(std::size_t partSize, std::uint32_t binding);

        /// @brief Copies a part of the size of a part into the memory of the current frame.
        /// @return The index of the part, or nullopt if the buffer cannot grow.
        virtual std::optional<std::uint32_t> Write(std::span<const std::byte> data) = 0;

        /// @brief Binds a part that the current frame added.
        virtual void BindPart(std::uint32_t part) = 0;

    private:
        std::size_t m_PartSize = 0;
        std::uint32_t m_Binding = 0;
    };

    /// @brief A storage block written again at each frame, as an array of elements of one size, such as the objects of
    /// the draws.
    ///
    /// It is created with GraphicsContext::CreateFrameStorageBuffer() with the size of an element and the binding of the
    /// block, the N of `layout(std430, binding = N) readonly buffer Objects { ... };`. Add() writes an element and gives
    /// its index in the array, and Bind() makes the next draws read the array of the current frame. The elements of a
    /// frame are kept until GraphicsContext::EndFrame(), then the next frame starts again from the element 0. As with
    /// FrameUniformBuffer, each frame writes into its own memory and the buffer grows when needed. The elements follow
    /// each other without padding, so the C++ struct must follow the std430 rules of GLSL.
    class FrameStorageBuffer
    {
    public:
        virtual ~FrameStorageBuffer() = default;

        FrameStorageBuffer(const FrameStorageBuffer&) = delete;
        FrameStorageBuffer& operator=(const FrameStorageBuffer&) = delete;
        FrameStorageBuffer(FrameStorageBuffer&&) = delete;
        FrameStorageBuffer& operator=(FrameStorageBuffer&&) = delete;

        /// @brief Returns true if elements of this size can be created, whatever the graphics API. It must not be 0. The
        /// reason a buffer is refused is logged as an engine error.
        static bool IsValid(std::size_t elementSize);

        /// @brief Writes an element for the current frame.
        /// @param data The bytes of the element, of the size of an element.
        /// @return The index of the element, or nullopt if the data has another size or if the buffer cannot grow. The
        /// reason is logged as an engine error.
        std::optional<std::uint32_t> AddData(std::span<const std::byte> data);

        /// @brief Writes an element from a value, usually a struct with the members of an element. See AddData().
        template <typename T>
        std::optional<std::uint32_t> Add(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "A frame storage buffer is filled with the bytes of the value");
            return AddData(std::as_bytes(std::span<const T, 1>(&value, 1)));
        }

        /// @brief Makes the next draws read the storage block of the binding of this buffer from the elements of the
        /// current frame.
        void Bind() { BindFrame(); }

        /// @brief Returns the number of elements that the current frame added.
        virtual std::uint32_t GetCount() const = 0;

        /// @brief Returns the size of an element, in bytes.
        std::size_t GetElementSize() const { return m_ElementSize; }

        /// @brief Returns the binding of the storage block that the buffer fills.
        std::uint32_t GetBinding() const { return m_Binding; }

    protected:
        /// @brief Keeps the size of an element, which must be valid, and the binding. See IsValid().
        FrameStorageBuffer(std::size_t elementSize, std::uint32_t binding);

        /// @brief Copies an element of the size of an element into the memory of the current frame.
        /// @return The index of the element, or nullopt if the buffer cannot grow.
        virtual std::optional<std::uint32_t> Write(std::span<const std::byte> data) = 0;

        /// @brief Binds the elements of the current frame.
        virtual void BindFrame() = 0;

    private:
        std::size_t m_ElementSize = 0;
        std::uint32_t m_Binding = 0;
    };
}
