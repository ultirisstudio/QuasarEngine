#include <platform/opengl/OpenGLBuffer.h>

#include <platform/opengl/OpenGLDevice.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace Quasar
{
    namespace
    {
        GLuint CreateStorage(const void* data, std::size_t size)
        {
            GLuint buffer = 0;
            glCreateBuffers(1, &buffer);
            glNamedBufferStorage(buffer, static_cast<GLsizeiptr>(size), data, GL_DYNAMIC_STORAGE_BIT);
            return buffer;
        }

        void WriteStorage(RenderStatistics& statistics, GLuint buffer, std::size_t offset, const void* data, std::size_t size)
        {
            ++statistics.Uploads;
            statistics.UploadedBytes += size;
            glNamedBufferSubData(buffer, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
        }

        bool FitsUniformLimits(const OpenGLLimits& limits, std::size_t size, std::uint32_t binding)
        {
            if (size > limits.UniformBlockSize)
            {
                QUASAR_ENGINE_ERROR("Cannot create a uniform buffer of {} bytes: OpenGL allows at most {}", size, limits.UniformBlockSize);
                return false;
            }
            if (binding >= limits.UniformBufferBindings)
            {
                QUASAR_ENGINE_ERROR("Cannot create a uniform buffer for the binding {}: OpenGL has the bindings 0 to {}", binding,
                                    limits.UniformBufferBindings - 1);
                return false;
            }
            return true;
        }

        std::size_t RoundUp(std::size_t size, std::size_t alignment)
        {
            return (size + alignment - 1) / alignment * alignment;
        }

        constexpr std::uint32_t FirstCapacity = 64;
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLDevice& device)
        : VertexBuffer(vertices.size(), layout), m_Device(&device), m_Buffer(CreateStorage(vertices.data(), vertices.size()))
    {
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLVertexBuffer> OpenGLVertexBuffer::Create(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLDevice& device)
    {
        if (!IsValid(vertices.size(), layout))
        {
            return nullptr;
        }
        return Scope<OpenGLVertexBuffer>(new OpenGLVertexBuffer(vertices, layout, device));
    }

    void OpenGLVertexBuffer::Write(std::span<const std::byte> data, std::size_t offset)
    {
        WriteStorage(m_Device->GetStatistics(), m_Buffer, offset, data.data(), data.size());
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(std::span<const std::uint32_t> indices, OpenGLDevice& device)
        : IndexBuffer(static_cast<std::uint32_t>(indices.size())), m_Device(&device), m_Buffer(CreateStorage(indices.data(), indices.size_bytes()))
    {
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLIndexBuffer> OpenGLIndexBuffer::Create(std::span<const std::uint32_t> indices, OpenGLDevice& device)
    {
        if (!IsValid(indices.size()))
        {
            return nullptr;
        }
        return Scope<OpenGLIndexBuffer>(new OpenGLIndexBuffer(indices, device));
    }

    void OpenGLIndexBuffer::Write(std::span<const std::uint32_t> indices, std::size_t first)
    {
        WriteStorage(m_Device->GetStatistics(), m_Buffer, first * sizeof(std::uint32_t), indices.data(), indices.size_bytes());
    }

    OpenGLUniformBuffer::OpenGLUniformBuffer(std::size_t size, std::uint32_t binding, OpenGLDevice& device)
        : UniformBuffer(size, binding), m_Device(&device), m_Buffer(CreateStorage(nullptr, size))
    {
        glClearNamedBufferData(m_Buffer, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        m_Device->GetState().ForgetBuffer(m_Buffer);
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLUniformBuffer> OpenGLUniformBuffer::Create(std::size_t size, std::uint32_t binding, OpenGLDevice& device)
    {
        if (!IsValid(size))
        {
            return nullptr;
        }

        if (!FitsUniformLimits(device.GetLimits(), size, binding))
        {
            return nullptr;
        }
        return Scope<OpenGLUniformBuffer>(new OpenGLUniformBuffer(size, binding, device));
    }

    void OpenGLUniformBuffer::Bind()
    {
        m_Device->GetState().BindUniformBuffer(GetBinding(), m_Buffer);
    }

    void OpenGLUniformBuffer::Write(std::span<const std::byte> data, std::size_t offset)
    {
        WriteStorage(m_Device->GetStatistics(), m_Buffer, offset, data.data(), data.size());
    }

    OpenGLFrameUniformBuffer::OpenGLFrameUniformBuffer(std::size_t partSize, std::uint32_t binding, OpenGLDevice& device)
        : FrameUniformBuffer(partSize, binding), m_Device(&device),
          m_Memory(device, RoundUp(partSize, device.GetLimits().UniformBufferOffsetAlignment), device.GetLimits().UniformBufferOffsetAlignment,
                   std::numeric_limits<std::size_t>::max(), "frame uniform buffer")
    {
    }

    Scope<OpenGLFrameUniformBuffer> OpenGLFrameUniformBuffer::Create(std::size_t partSize, std::uint32_t binding, OpenGLDevice& device)
    {
        if (!IsValid(partSize) || !FitsUniformLimits(device.GetLimits(), partSize, binding))
        {
            return nullptr;
        }
        Scope<OpenGLFrameUniformBuffer> buffer(new OpenGLFrameUniformBuffer(partSize, binding, device));
        if (!buffer->m_Memory.Allocate(FirstCapacity))
        {
            return nullptr;
        }
        return buffer;
    }

    void OpenGLFrameUniformBuffer::BindPart(std::uint32_t part)
    {
        const std::size_t offset = m_Memory.GetFrameOffset() + (part * m_Memory.GetStride());
        m_Device->GetState().BindUniformBufferRange(GetBinding(), m_Memory.GetBuffer(), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(GetPartSize()));
    }

    OpenGLFrameStorageBuffer::OpenGLFrameStorageBuffer(std::size_t elementSize, std::uint32_t binding, OpenGLDevice& device)
        : FrameStorageBuffer(elementSize, binding), m_Device(&device),
          m_Memory(device, elementSize, device.GetLimits().StorageBufferOffsetAlignment, device.GetLimits().StorageBlockSize, "frame storage buffer")
    {
    }

    Scope<OpenGLFrameStorageBuffer> OpenGLFrameStorageBuffer::Create(std::size_t elementSize, std::uint32_t binding, OpenGLDevice& device)
    {
        if (!IsValid(elementSize))
        {
            return nullptr;
        }
        const OpenGLLimits& limits = device.GetLimits();
        if (binding >= limits.StorageBufferBindings)
        {
            QUASAR_ENGINE_ERROR("Cannot create a storage buffer for the binding {}: OpenGL has the bindings 0 to {}", binding,
                                limits.StorageBufferBindings - 1);
            return nullptr;
        }
        Scope<OpenGLFrameStorageBuffer> buffer(new OpenGLFrameStorageBuffer(elementSize, binding, device));
        if (!buffer->m_Memory.Allocate(FirstCapacity))
        {
            return nullptr;
        }
        return buffer;
    }

    void OpenGLFrameStorageBuffer::BindFrame()
    {
        m_Device->GetState().BindStorageBufferRange(GetBinding(), m_Memory.GetBuffer(), static_cast<GLintptr>(m_Memory.GetFrameOffset()),
                                                    static_cast<GLsizeiptr>(m_Memory.GetZoneSize()));
    }
}
