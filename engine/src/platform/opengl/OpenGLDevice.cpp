#include <platform/opengl/OpenGLDevice.h>

#include <glad/gl.h>

#include <algorithm>
#include <string_view>

namespace Quasar
{
    namespace
    {
        constexpr GLuint64 FenceTimeout = 1'000'000'000;

        bool HasExtension(std::string_view name)
        {
            GLint count = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &count);
            for (GLint index = 0; index < count; ++index)
            {
                const GLubyte* extension = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(index));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                if (extension != nullptr && name == reinterpret_cast<const char*>(extension))
                {
                    return true;
                }
            }
            return false;
        }

        std::uint32_t ReadLimit(GLenum name)
        {
            GLint value = 0;
            glGetIntegerv(name, &value);
            return static_cast<std::uint32_t>(std::max(value, 0));
        }

        float ReadMaxAnisotropy()
        {
            if (GLAD_GL_VERSION_4_6 == 0 && !HasExtension("GL_ARB_texture_filter_anisotropic") && !HasExtension("GL_EXT_texture_filter_anisotropic"))
            {
                return 1.0f;
            }
            GLfloat value = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &value);
            return std::max(value, 1.0f);
        }

        OpenGLLimits ReadLimits()
        {
            return {.TextureSize = ReadLimit(GL_MAX_TEXTURE_SIZE),
                    .RenderbufferSize = ReadLimit(GL_MAX_RENDERBUFFER_SIZE),
                    .ColorAttachments = ReadLimit(GL_MAX_COLOR_ATTACHMENTS),
                    .Samples = ReadLimit(GL_MAX_SAMPLES),
                    .IntegerSamples = ReadLimit(GL_MAX_INTEGER_SAMPLES),
                    .ColorTextureSamples = ReadLimit(GL_MAX_COLOR_TEXTURE_SAMPLES),
                    .TextureSlots = ReadLimit(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
                    .UniformBlockSize = ReadLimit(GL_MAX_UNIFORM_BLOCK_SIZE),
                    .UniformBufferBindings = ReadLimit(GL_MAX_UNIFORM_BUFFER_BINDINGS),
                    .UniformBufferOffsetAlignment = std::max(ReadLimit(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT), 1U),
                    .Anisotropy = ReadMaxAnisotropy()};
        }
    }

    OpenGLDevice::OpenGLDevice(RenderStatistics& statistics) : m_Limits(ReadLimits()), m_State(statistics) {}

    OpenGLDevice::~OpenGLDevice()
    {
        for (GLsync fence : m_Fences)
        {
            glDeleteSync(fence);
        }
    }

    std::uint32_t OpenGLDevice::AcquireFrameMemory()
    {
        const auto zone = static_cast<std::uint32_t>(m_Frame % FramesInFlight);
        GLsync& fence = m_Fences.at(zone);
        if (!m_FrameMemoryAcquired && fence != nullptr)
        {
            while (glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, FenceTimeout) == GL_TIMEOUT_EXPIRED)
            {
            }
            glDeleteSync(fence);
            fence = nullptr;
        }
        m_FrameMemoryAcquired = true;
        return zone;
    }

    void OpenGLDevice::EndFrame()
    {
        if (m_FrameMemoryAcquired)
        {
            m_Fences.at(m_Frame % FramesInFlight) = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }
        m_FrameMemoryAcquired = false;
        ++m_Frame;
    }
}
