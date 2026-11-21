#include <platform/opengl/OpenGLFramebuffer.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    namespace
    {
        const char* GetFormatName(FramebufferFormat format)
        {
            switch (format)
            {
                case FramebufferFormat::Rgba8:
                    return "Rgba8";
                case FramebufferFormat::Rgba16F:
                    return "Rgba16F";
                case FramebufferFormat::RedInteger:
                    return "RedInteger";
                case FramebufferFormat::Depth24Stencil8:
                    return "Depth24Stencil8";
                case FramebufferFormat::Depth32F:
                    return "Depth32F";
            }
            return "unknown";
        }

        GLenum GetInternalFormat(FramebufferFormat format)
        {
            switch (format)
            {
                case FramebufferFormat::Rgba8:
                    return GL_RGBA8;
                case FramebufferFormat::Rgba16F:
                    return GL_RGBA16F;
                case FramebufferFormat::RedInteger:
                    return GL_R32I;
                case FramebufferFormat::Depth24Stencil8:
                    return GL_DEPTH24_STENCIL8;
                case FramebufferFormat::Depth32F:
                    return GL_DEPTH_COMPONENT32F;
            }
            return GL_RGBA8;
        }

        GLenum GetColorAttachment(std::size_t index)
        {
            return GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index);
        }

        int GetLimit(GLenum name)
        {
            GLint value = 0;
            glGetIntegerv(name, &value);
            return value;
        }

        GLuint CreateColorTexture(FramebufferFormat format, GLsizei width, GLsizei height, GLsizei samples)
        {
            GLuint texture = 0;
            if (samples > 1)
            {
                glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &texture);
                glTextureStorage2DMultisample(texture, samples, GetInternalFormat(format), width, height, GL_TRUE);
                return texture;
            }

            // Integer textures cannot be filtered linearly
            const GLint filter = format == FramebufferFormat::RedInteger ? GL_NEAREST : GL_LINEAR;
            glCreateTextures(GL_TEXTURE_2D, 1, &texture);
            glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, filter);
            glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, filter);
            glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureStorage2D(texture, 1, GetInternalFormat(format), width, height);
            return texture;
        }

        bool IsComplete(GLuint framebuffer)
        {
            const GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                QUASAR_ENGINE_ERROR("The OpenGL framebuffer is incomplete: status 0x{:X}", status);
                return false;
            }
            return true;
        }

        bool FitsTheLimits(const FramebufferSpecification& specification, std::size_t colorCount, bool integer)
        {
            const auto maxSize = static_cast<std::uint32_t>(std::min(GetLimit(GL_MAX_TEXTURE_SIZE), GetLimit(GL_MAX_RENDERBUFFER_SIZE)));
            if (specification.Width > maxSize || specification.Height > maxSize)
            {
                QUASAR_ENGINE_ERROR("Cannot create a framebuffer of {}x{} pixels: OpenGL allows at most {}x{}", specification.Width, specification.Height,
                                    maxSize, maxSize);
                return false;
            }

            const auto maxColors = static_cast<std::size_t>(GetLimit(GL_MAX_COLOR_ATTACHMENTS));
            if (colorCount > maxColors)
            {
                QUASAR_ENGINE_ERROR("Cannot create a framebuffer with {} color attachments: OpenGL allows at most {}", colorCount, maxColors);
                return false;
            }

            const auto maxSamples = static_cast<std::uint32_t>(GetLimit(integer ? GL_MAX_INTEGER_SAMPLES : GL_MAX_SAMPLES));
            if (specification.Samples > maxSamples)
            {
                QUASAR_ENGINE_ERROR("Cannot create a framebuffer with {} samples per pixel: OpenGL allows at most {}", specification.Samples, maxSamples);
                return false;
            }
            return true;
        }
    }

    void ClearOpenGLDepthStencil(std::uint32_t framebuffer, float depth, std::uint8_t stencil)
    {
        // A pipeline that does not write the depth also stops the depth from being cleared
        GLboolean depthWrite = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        glDepthMask(GL_TRUE);
        glClearNamedFramebufferfi(framebuffer, GL_DEPTH_STENCIL, 0, depth, static_cast<GLint>(stencil));
        glDepthMask(depthWrite);
    }

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& specification) : Framebuffer(specification)
    {
        for (const FramebufferFormat format : specification.Attachments)
        {
            if (format == FramebufferFormat::Depth24Stencil8 || format == FramebufferFormat::Depth32F)
            {
                m_DepthFormat = format;
            }
            else
            {
                m_ColorFormats.push_back(format);
            }
        }
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        DestroyTargets();
    }

    Scope<OpenGLFramebuffer> OpenGLFramebuffer::Create(const FramebufferSpecification& specification)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }

        Scope<OpenGLFramebuffer> framebuffer(new OpenGLFramebuffer(specification));
        const bool integer = std::ranges::find(framebuffer->m_ColorFormats, FramebufferFormat::RedInteger) != framebuffer->m_ColorFormats.end();
        if (!FitsTheLimits(specification, framebuffer->m_ColorFormats.size(), integer) || !framebuffer->CreateTargets())
        {
            return nullptr;
        }
        return framebuffer;
    }

    void OpenGLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0, static_cast<GLsizei>(GetSpecification().Width), static_cast<GLsizei>(GetSpecification().Height));
    }

    void OpenGLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Resolve();
    }

    void OpenGLFramebuffer::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || (width == GetSpecification().Width && height == GetSpecification().Height))
        {
            return;
        }

        FramebufferSpecification resized = GetSpecification();
        resized.Width = width;
        resized.Height = height;
        const bool integer = std::ranges::find(m_ColorFormats, FramebufferFormat::RedInteger) != m_ColorFormats.end();
        if (!FitsTheLimits(resized, m_ColorFormats.size(), integer))
        {
            return;
        }

        DestroyTargets();
        SetSize(width, height);
        CreateTargets();
    }

    void OpenGLFramebuffer::ClearColor(std::uint32_t attachment, glm::vec4 color)
    {
        if (const std::optional<std::size_t> index = FindColor(attachment, false, "fill with a color"))
        {
            glClearNamedFramebufferfv(m_Framebuffer, GL_COLOR, static_cast<GLint>(*index), glm::value_ptr(color));
        }
    }

    void OpenGLFramebuffer::ClearInteger(std::uint32_t attachment, int value)
    {
        if (const std::optional<std::size_t> index = FindColor(attachment, true, "fill with an integer"))
        {
            glClearNamedFramebufferiv(m_Framebuffer, GL_COLOR, static_cast<GLint>(*index), &value);
        }
    }

    void OpenGLFramebuffer::ClearDepthStencil(float depth, std::uint8_t stencil)
    {
        if (!m_DepthFormat)
        {
            QUASAR_ENGINE_ERROR("Cannot fill the depth of a framebuffer that has no depth attachment");
            return;
        }
        ClearOpenGLDepthStencil(m_Framebuffer, depth, stencil);
    }

    std::optional<int> OpenGLFramebuffer::ReadInteger(std::uint32_t attachment, std::uint32_t x, std::uint32_t y)
    {
        const std::optional<std::size_t> index = FindColor(attachment, true, "read an integer from");
        const FramebufferSpecification& specification = GetSpecification();
        if (!index || x >= specification.Width || y >= specification.Height)
        {
            return std::nullopt;
        }

        const std::uint32_t row = specification.Height - 1 - y;
        GLint value = 0;
        glGetTextureSubImage(GetReadableTexture(*index), 0, static_cast<GLint>(x), static_cast<GLint>(row), 0, 1, 1, 1, GL_RED_INTEGER, GL_INT, sizeof(value),
                             &value);
        return value;
    }

    void OpenGLFramebuffer::BindColorTexture(std::uint32_t attachment, std::uint32_t slot) const
    {
        if (const std::optional<std::size_t> index = FindColor(attachment, false, "bind"); index)
        {
            glBindTextureUnit(slot, GetReadableTexture(*index));
        }
    }

    void OpenGLFramebuffer::BindDepthTexture(std::uint32_t slot) const
    {
        if (HasDepthTexture())
        {
            glBindTextureUnit(slot, m_DepthTexture);
        }
    }

    void OpenGLFramebuffer::BindDepthValues(std::uint32_t slot) const
    {
        if (HasDepthTexture())
        {
            glBindTextureUnit(slot, m_DepthValues);
        }
    }

    bool OpenGLFramebuffer::HasDepthTexture() const
    {
        if (m_DepthFormat != FramebufferFormat::Depth32F)
        {
            QUASAR_ENGINE_ERROR("Cannot bind the depth of a framebuffer that has no Depth32F attachment");
            return false;
        }
        return true;
    }

    ImTextureID OpenGLFramebuffer::GetImGuiTexture(std::uint32_t attachment) const
    {
        const std::optional<std::size_t> index = FindColor(attachment, false, "show");
        return index ? static_cast<ImTextureID>(GetReadableTexture(*index)) : ImTextureID{};
    }

    bool OpenGLFramebuffer::CreateTargets()
    {
        const FramebufferSpecification& specification = GetSpecification();
        const auto width = static_cast<GLsizei>(specification.Width);
        const auto height = static_cast<GLsizei>(specification.Height);
        const auto samples = static_cast<GLsizei>(specification.Samples);

        glCreateFramebuffers(1, &m_Framebuffer);
        std::vector<GLenum> drawBuffers;
        for (std::size_t index = 0; index < m_ColorFormats.size(); ++index)
        {
            m_ColorTextures.push_back(CreateColorTexture(m_ColorFormats[index], width, height, samples));
            glNamedFramebufferTexture(m_Framebuffer, GetColorAttachment(index), m_ColorTextures.back(), 0);
            drawBuffers.push_back(GetColorAttachment(index));
        }

        if (drawBuffers.empty())
        {
            glNamedFramebufferDrawBuffer(m_Framebuffer, GL_NONE);
            glNamedFramebufferReadBuffer(m_Framebuffer, GL_NONE);
        }
        else
        {
            glNamedFramebufferDrawBuffers(m_Framebuffer, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        }

        if (m_DepthFormat == FramebufferFormat::Depth24Stencil8)
        {
            glCreateRenderbuffers(1, &m_DepthStencil);
            glNamedRenderbufferStorageMultisample(m_DepthStencil, samples > 1 ? samples : 0, GL_DEPTH24_STENCIL8, width, height);
            glNamedFramebufferRenderbuffer(m_Framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencil);
        }
        else if (m_DepthFormat == FramebufferFormat::Depth32F)
        {
            constexpr std::array<GLfloat, 4> outside{1.0f, 1.0f, 1.0f, 1.0f};
            glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthTexture);
            glTextureStorage2D(m_DepthTexture, 1, GL_DEPTH_COMPONENT32F, width, height);
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTextureParameterfv(m_DepthTexture, GL_TEXTURE_BORDER_COLOR, outside.data());
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            glNamedFramebufferTexture(m_Framebuffer, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0);

            // A view shares the storage of the depth with other settings, so that it is read as values
            glGenTextures(1, &m_DepthValues);
            glTextureView(m_DepthValues, GL_TEXTURE_2D, m_DepthTexture, GL_DEPTH_COMPONENT32F, 0, 1, 0, 1);
            glTextureParameteri(m_DepthValues, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(m_DepthValues, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(m_DepthValues, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_DepthValues, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        if (IsMultisampled() && !m_ColorFormats.empty())
        {
            glCreateFramebuffers(1, &m_ResolveFramebuffer);
            for (std::size_t index = 0; index < m_ColorFormats.size(); ++index)
            {
                m_ResolvedTextures.push_back(CreateColorTexture(m_ColorFormats[index], width, height, 1));
                glNamedFramebufferTexture(m_ResolveFramebuffer, GetColorAttachment(index), m_ResolvedTextures.back(), 0);
            }
            if (!IsComplete(m_ResolveFramebuffer))
            {
                return false;
            }
        }

        return IsComplete(m_Framebuffer);
    }

    void OpenGLFramebuffer::DestroyTargets()
    {
        glDeleteFramebuffers(1, &m_Framebuffer);
        glDeleteFramebuffers(1, &m_ResolveFramebuffer);
        glDeleteRenderbuffers(1, &m_DepthStencil);
        glDeleteTextures(1, &m_DepthValues);
        glDeleteTextures(1, &m_DepthTexture);
        glDeleteTextures(static_cast<GLsizei>(m_ColorTextures.size()), m_ColorTextures.data());
        glDeleteTextures(static_cast<GLsizei>(m_ResolvedTextures.size()), m_ResolvedTextures.data());

        m_Framebuffer = 0;
        m_ResolveFramebuffer = 0;
        m_DepthStencil = 0;
        m_DepthTexture = 0;
        m_DepthValues = 0;
        m_ColorTextures.clear();
        m_ResolvedTextures.clear();
    }

    void OpenGLFramebuffer::Resolve()
    {
        if (m_ResolveFramebuffer == 0)
        {
            return;
        }

        // The scissor test also limits the copy, so it is turned off to resolve the whole image
        const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
        glDisable(GL_SCISSOR_TEST);

        const auto width = static_cast<GLint>(GetSpecification().Width);
        const auto height = static_cast<GLint>(GetSpecification().Height);
        for (std::size_t index = 0; index < m_ColorFormats.size(); ++index)
        {
            glNamedFramebufferReadBuffer(m_Framebuffer, GetColorAttachment(index));
            glNamedFramebufferDrawBuffer(m_ResolveFramebuffer, GetColorAttachment(index));
            glBlitNamedFramebuffer(m_Framebuffer, m_ResolveFramebuffer, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        if (scissor == GL_TRUE)
        {
            glEnable(GL_SCISSOR_TEST);
        }
    }

    bool OpenGLFramebuffer::IsMultisampled() const
    {
        return GetSpecification().Samples > 1;
    }

    std::optional<std::size_t> OpenGLFramebuffer::FindColor(std::uint32_t attachment, bool integer, const char* action) const
    {
        if (attachment >= m_ColorFormats.size())
        {
            QUASAR_ENGINE_ERROR("Cannot {} the color attachment {} of the framebuffer: it has {} color attachments", action, attachment, m_ColorFormats.size());
            return std::nullopt;
        }

        const FramebufferFormat format = m_ColorFormats[attachment];
        if ((format == FramebufferFormat::RedInteger) != integer)
        {
            QUASAR_ENGINE_ERROR("Cannot {} the color attachment {} of the framebuffer: its format is {}", action, attachment, GetFormatName(format));
            return std::nullopt;
        }
        return attachment;
    }

    std::uint32_t OpenGLFramebuffer::GetReadableTexture(std::size_t index) const
    {
        return IsMultisampled() ? m_ResolvedTextures.at(index) : m_ColorTextures.at(index);
    }
}
