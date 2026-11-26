#pragma once

#include <platform/opengl/OpenGLState.h>

#include <quasar/renderer/RenderStatistics.h>

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
        float Anisotropy = 1.0f;
    };

    class OpenGLDevice
    {
    public:
        explicit OpenGLDevice(RenderStatistics& statistics);

        OpenGLState& GetState() { return m_State; }
        RenderStatistics& GetStatistics() const { return m_State.GetStatistics(); }
        const OpenGLLimits& GetLimits() const { return m_Limits; }

    private:
        OpenGLLimits m_Limits;
        OpenGLState m_State;
    };
}
