#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/VertexLayout.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    class OpenGLVertexBuffer final : public VertexBuffer
    {
    public:
        static Scope<OpenGLVertexBuffer> Create(std::span<const std::byte> vertices, const VertexLayout& layout);

        ~OpenGLVertexBuffer() override;

        OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;
        OpenGLVertexBuffer& operator=(const OpenGLVertexBuffer&) = delete;
        OpenGLVertexBuffer(OpenGLVertexBuffer&&) = delete;
        OpenGLVertexBuffer& operator=(OpenGLVertexBuffer&&) = delete;

        std::uint32_t GetId() const { return m_Buffer; }

    private:
        OpenGLVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout);

        void Write(std::span<const std::byte> data, std::size_t offset) override;

        std::uint32_t m_Buffer = 0;
    };

    class OpenGLIndexBuffer final : public IndexBuffer
    {
    public:
        static Scope<OpenGLIndexBuffer> Create(std::span<const std::uint32_t> indices);

        ~OpenGLIndexBuffer() override;

        OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
        OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer&) = delete;
        OpenGLIndexBuffer(OpenGLIndexBuffer&&) = delete;
        OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&&) = delete;

        std::uint32_t GetId() const { return m_Buffer; }

    private:
        explicit OpenGLIndexBuffer(std::span<const std::uint32_t> indices);

        void Write(std::span<const std::uint32_t> indices, std::size_t first) override;

        std::uint32_t m_Buffer = 0;
    };

    class OpenGLUniformBuffer final : public UniformBuffer
    {
    public:
        static Scope<OpenGLUniformBuffer> Create(std::size_t size, std::uint32_t binding);

        ~OpenGLUniformBuffer() override;

        OpenGLUniformBuffer(const OpenGLUniformBuffer&) = delete;
        OpenGLUniformBuffer& operator=(const OpenGLUniformBuffer&) = delete;
        OpenGLUniformBuffer(OpenGLUniformBuffer&&) = delete;
        OpenGLUniformBuffer& operator=(OpenGLUniformBuffer&&) = delete;

        void Bind() override;

    private:
        OpenGLUniformBuffer(std::size_t size, std::uint32_t binding);

        void Write(std::span<const std::byte> data, std::size_t offset) override;

        std::uint32_t m_Buffer = 0;
    };
}
