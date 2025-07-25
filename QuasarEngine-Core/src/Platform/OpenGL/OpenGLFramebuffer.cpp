#include "qepch.h"
#include "OpenGLFramebuffer.h"

#include <glad/glad.h>

#include "QuasarEngine/Renderer/RenderCommand.h"

namespace QuasarEngine
{
    namespace Utils
    {
        static GLenum TextureTarget(bool multisampled)
        {
            return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        }

        static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
        {
            glCreateTextures(TextureTarget(multisampled), count, outID);
        }

        static void BindTexture(bool multisampled, uint32_t id)
        {
            glBindTexture(TextureTarget(multisampled), id);
        }

        static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
        {
            bool multisampled = samples > 1;
            if (multisampled)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
        }

        static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
        {
            bool multisampled = samples > 1;
            if (multisampled)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            }
            else
            {
                glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
        }

        static bool isDepthFormat(FramebufferTextureFormat format)
        {
            switch (format)
            {
            case FramebufferTextureFormat::DEPTH24STENCIL8:
                return true;
            default:
                return false;
            }
        }

        static GLenum FramebufferTextureFormatToGL(FramebufferTextureFormat format)
        {
            switch (format)
            {
            case FramebufferTextureFormat::RGBA8:			return GL_RGBA8;
            case FramebufferTextureFormat::RED_INTEGER:		return GL_RED_INTEGER;
            case FramebufferTextureFormat::DEPTH24STENCIL8:	return GL_DEPTH24_STENCIL8;
            default:
                std::cerr << "Unknown texture format" << std::endl;
                return 0;
            }
        }
    }

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec) : Framebuffer(spec), m_ID(0)
    {
        for (auto format : spec.Attachments.Attachments)
        {
            if (!Utils::isDepthFormat(format.TextureFormat))
                m_ColorAttachmentSpecifications.emplace_back(format);
            else
                m_DepthAttachmentSpecification = format;
        }
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        glDeleteFramebuffers(1, &m_ID);
        glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
        glDeleteTextures(1, &m_DepthAttachment);
    }

    void* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_ColorAttachments.size()) return nullptr;
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ColorAttachments[index]));
    }

    void* OpenGLFramebuffer::GetDepthAttachment() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_DepthAttachment));
    }

    int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        if (attachmentIndex >= m_ColorAttachments.size())
            return -1;

        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        int pixelData;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        return pixelData;
    }

    void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
    {
        auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
        glClearTexImage(m_ColorAttachments[attachmentIndex], 0, Utils::FramebufferTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
    }

    void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (m_Specification.Width == width && m_Specification.Height == height)
            return;

        if (width == 0 || height == 0)
            return;

        m_Specification.Width = width;
        m_Specification.Height = height;

        Invalidate();
    }

    void OpenGLFramebuffer::Invalidate()
    {
        if (m_Specification.Width == 0 && m_Specification.Height == 0)
        {
            std::cerr << "Width or height equal 0 !" << std::endl;
            return;
        }

        if (m_ID)
        {
            glDeleteFramebuffers(1, &m_ID);
            glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
            glDeleteTextures(1, &m_DepthAttachment);

            m_ColorAttachments.clear();
            m_DepthAttachment = 0;
        }

        glCreateFramebuffers(1, &m_ID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        bool multisample = m_Specification.Samples > 1;

        if (m_ColorAttachmentSpecifications.size())
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());

            Utils::CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

            for (size_t i = 0; i < m_ColorAttachmentSpecifications.size(); i++)
            {
                Utils::BindTexture(multisample, m_ColorAttachments[i]);
                switch (m_ColorAttachmentSpecifications[i].TextureFormat)
                {
                case FramebufferTextureFormat::RGBA8:
                    Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_RGBA8, GL_RGBA, m_Specification.Width, m_Specification.Height, i);
                    break;
                case FramebufferTextureFormat::RED_INTEGER:
                    Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_R32I, GL_RED_INTEGER, m_Specification.Width, m_Specification.Height, i);
                    break;
                }
            }
        }

        if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
        {
            Utils::CreateTextures(multisample, &m_DepthAttachment, 1);
            Utils::BindTexture(multisample, m_DepthAttachment);
            switch (m_DepthAttachmentSpecification.TextureFormat)
            {
            case FramebufferTextureFormat::DEPTH24STENCIL8:
                Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Specification.Width, m_Specification.Height);
                break;
            }
        }

        if (m_ColorAttachments.size() > 1)
        {
            if (m_ColorAttachments.size() > 4)
                std::cout << "Framebuffer has more than 4 attachments!" << std::endl;

            std::vector<GLenum> buffers(m_ColorAttachments.size());
            for (size_t i = 0; i < buffers.size(); ++i)
                buffers[i] = GL_COLOR_ATTACHMENT0 + i;
            glDrawBuffers((GLsizei)buffers.size(), buffers.data());
        }
        else if (m_ColorAttachments.empty())
        {
            glDrawBuffer(GL_NONE);
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "[Framebuffer] Incomplete! Status = 0x" << std::hex << status << std::dec << std::endl;
            switch (status)
            {
            case GL_FRAMEBUFFER_UNDEFINED: std::cerr << "GL_FRAMEBUFFER_UNDEFINED\n"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n"; break;
            case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED\n"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n"; break;
            default: std::cerr << "Unknown error\n"; break;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (multisample)
        {
            if (m_ResolveFBO)
            {
                glDeleteFramebuffers(1, &m_ResolveFBO);
                glDeleteTextures(1, &m_ResolvedColorTexture);
            }

            glGenTextures(1, &m_ResolvedColorTexture);
            glBindTexture(GL_TEXTURE_2D, m_ResolvedColorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Specification.Width, m_Specification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenFramebuffers(1, &m_ResolveFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolvedColorTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "[Resolve FBO] Incomplete!" << std::endl;
        }

    }

    void OpenGLFramebuffer::Resolve()
    {
        if (m_Specification.Samples <= 1) return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFBO);

        glBlitFramebuffer(
            0, 0, m_Specification.Width, m_Specification.Height,
            0, 0, m_Specification.Width, m_Specification.Height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        
        RenderCommand::SetViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }

    void OpenGLFramebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::BindColorAttachment(uint32_t index) const
    {
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[index]);
    }
}