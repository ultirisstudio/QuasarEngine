#pragma once

#include <platform/opengl/OpenGLState.h>

#include <quasar/renderer/RenderStatistics.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace Quasar
{
    struct OpenGLLimits
    {
        std::uint32_t TextureSize = 0;
        std::uint32_t RenderbufferSize = 0;
        std::uint32_t ColorAttachments = 0;
        std::uint32_t Samples = 1;
        std::uint32_t IntegerSamples = 1;
        std::uint32_t ColorTextureSamples = 1;
        std::uint32_t TextureSlots = 0;
        std::size_t UniformBlockSize = 0;
        std::uint32_t UniformBufferBindings = 0;
        std::uint32_t UniformBufferOffsetAlignment = 1;
        std::size_t StorageBlockSize = 0;
        std::uint32_t StorageBufferBindings = 0;
        std::uint32_t StorageBufferOffsetAlignment = 1;
        float Anisotropy = 1.0f;
    };

    class OpenGLDevice
    {
    public:
        static constexpr std::uint32_t FramesInFlight = 3;

        explicit OpenGLDevice(RenderStatistics& statistics);
        ~OpenGLDevice();

        OpenGLDevice(const OpenGLDevice&) = delete;
        OpenGLDevice& operator=(const OpenGLDevice&) = delete;
        OpenGLDevice(OpenGLDevice&&) = delete;
        OpenGLDevice& operator=(OpenGLDevice&&) = delete;

        OpenGLState& GetState() { return m_State; }
        RenderStatistics& GetStatistics() const { return m_State.GetStatistics(); }
        const OpenGLLimits& GetLimits() const { return m_Limits; }

        std::uint64_t GetFrame() const { return m_Frame; }
        std::uint32_t AcquireFrameMemory();
        void EndFrame();

    private:
        OpenGLLimits m_Limits;
        OpenGLState m_State;
        std::uint64_t m_Frame = 0;
        bool m_FrameMemoryAcquired = false;
        std::array<GLsync, FramesInFlight> m_Fences{};
    };
}
