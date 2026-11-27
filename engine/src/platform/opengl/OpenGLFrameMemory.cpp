#include <platform/opengl/OpenGLFrameMemory.h>

#include <platform/opengl/OpenGLDevice.h>

#include <quasar/log/Log.h>

#include <algorithm>

namespace Quasar
{
    namespace
    {
        constexpr GLbitfield MappingFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

        std::size_t RoundUp(std::size_t size, std::size_t alignment)
        {
            return (size + alignment - 1) / alignment * alignment;
        }
    }

    OpenGLFrameMemory::OpenGLFrameMemory(OpenGLDevice& device, std::size_t stride, std::size_t zoneAlignment, std::size_t maxZoneSize, std::string_view name)
        : m_Device(&device), m_Stride(stride), m_ZoneAlignment(zoneAlignment), m_MaxZoneSize(maxZoneSize), m_Name(name), m_Frame(device.GetFrame())
    {
    }

    OpenGLFrameMemory::~OpenGLFrameMemory()
    {
        if (m_Buffer != 0)
        {
            m_Device->GetState().ForgetBuffer(m_Buffer);
            glUnmapNamedBuffer(m_Buffer);
            glDeleteBuffers(1, &m_Buffer);
        }
    }

    bool OpenGLFrameMemory::Allocate(std::uint32_t capacity)
    {
        const std::size_t zoneSize = RoundUp(m_Stride * capacity, m_ZoneAlignment);
        if (zoneSize > m_MaxZoneSize)
        {
            QUASAR_ENGINE_ERROR("Cannot grow a {} to {} bytes per frame: OpenGL allows at most {}", m_Name, zoneSize, m_MaxZoneSize);
            return false;
        }
        const std::size_t size = zoneSize * OpenGLDevice::FramesInFlight;
        GLuint buffer = 0;
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, static_cast<GLsizeiptr>(size), nullptr, MappingFlags);
        void* memory = glMapNamedBufferRange(buffer, 0, static_cast<GLsizeiptr>(size), MappingFlags);
        if (memory == nullptr)
        {
            glDeleteBuffers(1, &buffer);
            QUASAR_ENGINE_ERROR("Cannot create a {} of {} bytes: OpenGL cannot map it", m_Name, size);
            return false;
        }

        if (m_Buffer != 0)
        {
            if (m_Count > 0)
            {
                glCopyNamedBufferSubData(m_Buffer, buffer, static_cast<GLintptr>(GetFrameOffset()), static_cast<GLintptr>(m_Zone * zoneSize),
                                         static_cast<GLsizeiptr>(m_Count * m_Stride));
            }
            m_Device->GetState().ForgetBuffer(m_Buffer);
            glUnmapNamedBuffer(m_Buffer);
            glDeleteBuffers(1, &m_Buffer);
        }
        m_Buffer = buffer;
        m_Memory = std::span(static_cast<std::byte*>(memory), size);
        m_Capacity = capacity;
        m_ZoneSize = zoneSize;
        return true;
    }

    std::optional<std::uint32_t> OpenGLFrameMemory::Write(std::span<const std::byte> data)
    {
        if (m_Frame != m_Device->GetFrame())
        {
            m_Frame = m_Device->GetFrame();
            m_Count = 0;
        }
        if (m_Count == 0)
        {
            m_Zone = m_Device->AcquireFrameMemory();
        }
        if (m_Count == m_Capacity && !Allocate(m_Capacity * 2))
        {
            return std::nullopt;
        }

        std::ranges::copy(data, m_Memory.subspan(GetFrameOffset() + (m_Count * m_Stride)).begin());
        m_Device->GetStatistics().UploadedBytes += data.size();
        return m_Count++;
    }

    std::uint32_t OpenGLFrameMemory::GetCount() const
    {
        return m_Frame == m_Device->GetFrame() ? m_Count : 0;
    }
}
