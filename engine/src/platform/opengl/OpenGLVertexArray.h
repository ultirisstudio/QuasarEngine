#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/VertexArray.h>

#include <cstdint>

namespace Quasar
{
    class OpenGLDevice;

    class OpenGLVertexArray final : public VertexArray
    {
    public:
        static Scope<OpenGLVertexArray> Create(OpenGLDevice& device);

        ~OpenGLVertexArray() override;

        OpenGLVertexArray(const OpenGLVertexArray&) = delete;
        OpenGLVertexArray& operator=(const OpenGLVertexArray&) = delete;
        OpenGLVertexArray(OpenGLVertexArray&&) = delete;
        OpenGLVertexArray& operator=(OpenGLVertexArray&&) = delete;

        void Bind() const;

    private:
        OpenGLVertexArray(std::uint32_t vertexArray, OpenGLDevice& device);

        void AttachVertexBuffer(const VertexBuffer& buffer, std::uint32_t index) override;
        void AttachIndexBuffer(const IndexBuffer& buffer) override;

        std::uint32_t m_VertexArray = 0;
        OpenGLDevice* m_Device;
    };
}
