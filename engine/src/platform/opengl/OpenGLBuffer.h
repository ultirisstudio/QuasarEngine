#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/VertexLayout.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace Quasar
{
    class OpenGLDevice;

    class OpenGLVertexBuffer final : public VertexBuffer
    {
    public:
        static Scope<OpenGLVertexBuffer> Create(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLDevice& device);

        ~OpenGLVertexBuffer() override;

        OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;
        OpenGLVertexBuffer& operator=(const OpenGLVertexBuffer&) = delete;
        OpenGLVertexBuffer(OpenGLVertexBuffer&&) = delete;
        OpenGLVertexBuffer& operator=(OpenGLVertexBuffer&&) = delete;

        std::uint32_t GetId() const { return m_Buffer; }

    private:
        OpenGLVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLDevice& device);

        void Write(std::span<const std::byte> data, std::size_t offset) override;

        OpenGLDevice* m_Device;
        std::uint32_t m_Buffer = 0;
    };

    class OpenGLIndexBuffer final : public IndexBuffer
    {
    public:
        static Scope<OpenGLIndexBuffer> Create(std::span<const std::uint32_t> indices, OpenGLDevice& device);

        ~OpenGLIndexBuffer() override;

        OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
        OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer&) = delete;
        OpenGLIndexBuffer(OpenGLIndexBuffer&&) = delete;
        OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&&) = delete;

        std::uint32_t GetId() const { return m_Buffer; }

    private:
        OpenGLIndexBuffer(std::span<const std::uint32_t> indices, OpenGLDevice& device);

        void Write(std::span<const std::uint32_t> indices, std::size_t first) override;

        OpenGLDevice* m_Device;
        std::uint32_t m_Buffer = 0;
    };

    class OpenGLUniformBuffer final : public UniformBuffer
    {
    public:
        static Scope<OpenGLUniformBuffer> Create(std::size_t size, std::uint32_t binding, OpenGLDevice& device);

        ~OpenGLUniformBuffer() override;

        OpenGLUniformBuffer(const OpenGLUniformBuffer&) = delete;
        OpenGLUniformBuffer& operator=(const OpenGLUniformBuffer&) = delete;
        OpenGLUniformBuffer(OpenGLUniformBuffer&&) = delete;
        OpenGLUniformBuffer& operator=(OpenGLUniformBuffer&&) = delete;

        void Bind() override;

    private:
        OpenGLUniformBuffer(std::size_t size, std::uint32_t binding, OpenGLDevice& device);

        void Write(std::span<const std::byte> data, std::size_t offset) override;

        OpenGLDevice* m_Device;
        std::uint32_t m_Buffer = 0;
    };

    class OpenGLFrameUniformBuffer final : public FrameUniformBuffer
    {
    public:
        static Scope<OpenGLFrameUniformBuffer> Create(std::size_t partSize, std::uint32_t binding, OpenGLDevice& device);

        ~OpenGLFrameUniformBuffer() override;

        OpenGLFrameUniformBuffer(const OpenGLFrameUniformBuffer&) = delete;
        OpenGLFrameUniformBuffer& operator=(const OpenGLFrameUniformBuffer&) = delete;
        OpenGLFrameUniformBuffer(OpenGLFrameUniformBuffer&&) = delete;
        OpenGLFrameUniformBuffer& operator=(OpenGLFrameUniformBuffer&&) = delete;

        std::uint32_t GetPartCount() const override;

    private:
        OpenGLFrameUniformBuffer(std::size_t partSize, std::uint32_t binding, OpenGLDevice& device);

        std::optional<std::uint32_t> Write(std::span<const std::byte> data) override;
        void BindPart(std::uint32_t part) override;

        bool Allocate(std::uint32_t capacity);
        std::size_t GetFrameOffset(std::uint32_t zone) const;

        OpenGLDevice* m_Device;
        std::size_t m_Stride = 0;
        std::uint32_t m_Capacity = 0;
        std::uint32_t m_Buffer = 0;
        std::span<std::byte> m_Memory;
        std::uint64_t m_Frame = 0;
        std::uint32_t m_Zone = 0;
        std::uint32_t m_Count = 0;
    };
}
