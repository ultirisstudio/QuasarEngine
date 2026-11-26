#include <platform/opengl/OpenGLBuffer.h>

#include <platform/opengl/OpenGLState.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
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

        std::size_t GetLimit(GLenum name)
        {
            GLint value = 0;
            glGetIntegerv(name, &value);
            return static_cast<std::size_t>(value);
        }
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLState& state)
        : VertexBuffer(vertices.size(), layout), m_State(&state), m_Buffer(CreateStorage(vertices.data(), vertices.size()))
    {
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLVertexBuffer> OpenGLVertexBuffer::Create(std::span<const std::byte> vertices, const VertexLayout& layout, OpenGLState& state)
    {
        if (!IsValid(vertices.size(), layout))
        {
            return nullptr;
        }
        return Scope<OpenGLVertexBuffer>(new OpenGLVertexBuffer(vertices, layout, state));
    }

    void OpenGLVertexBuffer::Write(std::span<const std::byte> data, std::size_t offset)
    {
        WriteStorage(m_State->GetStatistics(), m_Buffer, offset, data.data(), data.size());
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(std::span<const std::uint32_t> indices, OpenGLState& state)
        : IndexBuffer(static_cast<std::uint32_t>(indices.size())), m_State(&state), m_Buffer(CreateStorage(indices.data(), indices.size_bytes()))
    {
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLIndexBuffer> OpenGLIndexBuffer::Create(std::span<const std::uint32_t> indices, OpenGLState& state)
    {
        if (!IsValid(indices.size()))
        {
            return nullptr;
        }
        return Scope<OpenGLIndexBuffer>(new OpenGLIndexBuffer(indices, state));
    }

    void OpenGLIndexBuffer::Write(std::span<const std::uint32_t> indices, std::size_t first)
    {
        WriteStorage(m_State->GetStatistics(), m_Buffer, first * sizeof(std::uint32_t), indices.data(), indices.size_bytes());
    }

    OpenGLUniformBuffer::OpenGLUniformBuffer(std::size_t size, std::uint32_t binding, OpenGLState& state)
        : UniformBuffer(size, binding), m_State(&state), m_Buffer(CreateStorage(nullptr, size))
    {
        glClearNamedBufferData(m_Buffer, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        m_State->ForgetBuffer(m_Buffer);
        glDeleteBuffers(1, &m_Buffer);
    }

    Scope<OpenGLUniformBuffer> OpenGLUniformBuffer::Create(std::size_t size, std::uint32_t binding, OpenGLState& state)
    {
        if (!IsValid(size))
        {
            return nullptr;
        }

        const std::size_t maxSize = GetLimit(GL_MAX_UNIFORM_BLOCK_SIZE);
        if (size > maxSize)
        {
            QUASAR_ENGINE_ERROR("Cannot create a uniform buffer of {} bytes: OpenGL allows at most {}", size, maxSize);
            return nullptr;
        }
        const std::size_t bindingCount = GetLimit(GL_MAX_UNIFORM_BUFFER_BINDINGS);
        if (binding >= bindingCount)
        {
            QUASAR_ENGINE_ERROR("Cannot create a uniform buffer for the binding {}: OpenGL has the bindings 0 to {}", binding, bindingCount - 1);
            return nullptr;
        }
        return Scope<OpenGLUniformBuffer>(new OpenGLUniformBuffer(size, binding, state));
    }

    void OpenGLUniformBuffer::Bind()
    {
        m_State->BindUniformBuffer(GetBinding(), m_Buffer);
    }

    void OpenGLUniformBuffer::Write(std::span<const std::byte> data, std::size_t offset)
    {
        WriteStorage(m_State->GetStatistics(), m_Buffer, offset, data.data(), data.size());
    }
}
