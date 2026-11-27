#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace Quasar
{
    class OpenGLDevice;

    class OpenGLFrameMemory
    {
    public:
        OpenGLFrameMemory(OpenGLDevice& device, std::size_t stride, std::size_t zoneAlignment, std::size_t maxZoneSize, std::string_view name);
        ~OpenGLFrameMemory();

        OpenGLFrameMemory(const OpenGLFrameMemory&) = delete;
        OpenGLFrameMemory& operator=(const OpenGLFrameMemory&) = delete;
        OpenGLFrameMemory(OpenGLFrameMemory&&) = delete;
        OpenGLFrameMemory& operator=(OpenGLFrameMemory&&) = delete;

        bool Allocate(std::uint32_t capacity);
        std::optional<std::uint32_t> Write(std::span<const std::byte> data);

        std::uint32_t GetCount() const;
        GLuint GetBuffer() const { return m_Buffer; }
        std::size_t GetStride() const { return m_Stride; }
        std::size_t GetFrameOffset() const { return m_Zone * m_ZoneSize; }
        std::size_t GetZoneSize() const { return m_ZoneSize; }

    private:
        OpenGLDevice* m_Device;
        std::size_t m_Stride;
        std::size_t m_ZoneAlignment;
        std::size_t m_MaxZoneSize;
        std::string_view m_Name;
        std::uint32_t m_Capacity = 0;
        std::size_t m_ZoneSize = 0;
        GLuint m_Buffer = 0;
        std::span<std::byte> m_Memory;
        std::uint64_t m_Frame = 0;
        std::uint32_t m_Zone = 0;
        std::uint32_t m_Count = 0;
    };
}
