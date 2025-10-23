#include "qepch.h"
#include "OpenGLFramebuffer.h"

#include "OpenGLTexture2DView.h"
#include <QuasarEngine/Resources/Texture.h>
#include <QuasarEngine/Resources/TextureArray.h>
#include <QuasarEngine/Resources/TextureCubeMap.h>

#include <QuasarEngine/Core/Logger.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

#include <glad/glad.h>
#include <algorithm>

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
            const bool multisampled = samples > 1;
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
            const bool multisampled = samples > 1;
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

        static bool IsDepthFormat(FramebufferTextureFormat format)
        {
            switch (format)
            {
            case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
            default: return false;
            }
        }

        static GLenum FramebufferTextureFormatToGL(FramebufferTextureFormat format)
        {
            switch (format)
            {
            case FramebufferTextureFormat::RGBA8:           return GL_RGBA8;
            case FramebufferTextureFormat::RED_INTEGER:     return GL_R32I;
            case FramebufferTextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
            default:
                Q_ERROR("Unknown framebuffer texture format!");
                return 0;
            }
        }

        static void GLBaseFormatType(FramebufferTextureFormat fmt, GLenum& base, GLenum& type)
        {
            switch (fmt)
            {
            case FramebufferTextureFormat::RGBA8:
                base = GL_RGBA; type = GL_FLOAT;
                return;
            case FramebufferTextureFormat::RED_INTEGER:
                base = GL_RED_INTEGER; type = GL_INT;
                return;
            default:
                base = GL_RGBA; type = GL_FLOAT;
                return;
            }
        }
    }

    void OpenGLFramebuffer::DestroyInternalTargets()
    {
        if (m_ID) { glDeleteFramebuffers(1, &m_ID); m_ID = 0; }
        if (!m_ColorAttachments.empty())
        {
            glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
            m_ColorAttachments.clear();
        }
        if (m_DepthAttachment) { glDeleteTextures(1, &m_DepthAttachment); m_DepthAttachment = 0; }
    }

    void OpenGLFramebuffer::DestroyResolveTargets()
    {
        if (!m_ResolveFBOs.empty())
        {
            glDeleteFramebuffers((GLsizei)m_ResolveFBOs.size(), m_ResolveFBOs.data());
            m_ResolveFBOs.clear();
        }
        if (!m_ResolvedColorTextures.empty())
        {
            glDeleteTextures((GLsizei)m_ResolvedColorTextures.size(), m_ResolvedColorTextures.data());
            m_ResolvedColorTextures.clear();
        }
        m_ResolvedColorViews.clear();
    }

    void OpenGLFramebuffer::CreateResolveTargetsIfNeeded()
    {
        const bool multisample = m_Specification.Samples > 1;
        if (!multisample) return;

        DestroyResolveTargets();

        const size_t count = m_ColorAttachments.size();
        if (count == 0) return;

        m_ResolveFBOs.resize(count, 0);
        m_ResolvedColorTextures.resize(count, 0);
        glCreateFramebuffers((GLsizei)count, m_ResolveFBOs.data());
        glCreateTextures(GL_TEXTURE_2D, (GLsizei)count, m_ResolvedColorTextures.data());

        for (size_t i = 0; i < count; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, m_ResolvedColorTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                m_Specification.Width, m_Specification.Height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBOs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolvedColorTextures[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                Q_ERROR("Failed to create resolve framebuffer for attachment %zu!", i);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
        : Framebuffer(spec)
    {
        for (auto format : spec.Attachments.Attachments)
        {
            if (!Utils::IsDepthFormat(format.TextureFormat))
                m_ColorAttachmentSpecifications.emplace_back(format);
            else
                m_DepthAttachmentSpecification = format;
        }
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        DestroyResolveTargets();
        DestroyInternalTargets();
    }

    void OpenGLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        RenderCommand::Instance().SetViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }

    void OpenGLFramebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if ((m_Specification.Width == width && m_Specification.Height == height) || width == 0 || height == 0)
            return;

        m_Specification.Width = width;
        m_Specification.Height = height;
        Invalidate();
    }

    void OpenGLFramebuffer::Invalidate()
    {
        if (m_Specification.Width == 0 || m_Specification.Height == 0)
        {
            Q_ERROR("Attempted to create framebuffer with 0 width or height!");
            return;
        }

        DestroyResolveTargets();
        DestroyInternalTargets();

        glCreateFramebuffers(1, &m_ID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        const bool multisample = m_Specification.Samples > 1;

        if (!m_ColorAttachmentSpecifications.empty())
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
            Utils::CreateTextures(multisample, m_ColorAttachments.data(), (uint32_t)m_ColorAttachments.size());

            for (size_t i = 0; i < m_ColorAttachmentSpecifications.size(); ++i)
            {
                Utils::BindTexture(multisample, m_ColorAttachments[i]);
                switch (m_ColorAttachmentSpecifications[i].TextureFormat)
                {
                case FramebufferTextureFormat::RGBA8:
                    Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_RGBA8, GL_RGBA,
                        m_Specification.Width, m_Specification.Height, (int)i);
                    break;
                case FramebufferTextureFormat::RED_INTEGER:
                    Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_R32I, GL_RED_INTEGER,
                        m_Specification.Width, m_Specification.Height, (int)i);
                    break;
                default: break;
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
                Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH24_STENCIL8,
                    GL_DEPTH_STENCIL_ATTACHMENT, m_Specification.Width, m_Specification.Height);
                break;
            default: break;
            }
        }

        UpdateDrawBuffers();

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Q_ERROR("Framebuffer incomplete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        CreateResolveTargetsIfNeeded();

        m_ColorAttachmentViews.clear();
        m_ResolvedColorViews.clear();
    }

    void OpenGLFramebuffer::Resolve()
    {
        if (m_Specification.Samples <= 1) return;
        if (m_ColorAttachments.empty()) return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID);
        for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
        {
            if (m_ResolveFBOs.size() <= i || m_ResolveFBOs[i] == 0) continue;
            glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)i);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFBOs[i]);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            glBlitFramebuffer(0, 0, (GLint)m_Specification.Width, (GLint)m_Specification.Height,
                0, 0, (GLint)m_Specification.Width, (GLint)m_Specification.Height,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index < m_ColorAttachments.size())
            return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ColorAttachments[index]));

        if (index < m_ExternalColorAttachments.size())
        {
            const auto& ar = m_ExternalColorAttachments[index];
            if (ar.texture.get())
            {
                const auto id = static_cast<uint32_t>(ar.texture->GetHandle());
                return reinterpret_cast<void*>(static_cast<uintptr_t>(id));
            }
        }
        return nullptr;
    }

    void* OpenGLFramebuffer::GetDepthAttachment() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_DepthAttachment));
    }

    int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        int pixelData = 0;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return pixelData;
    }

    void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
    {
        if (attachmentIndex < m_ColorAttachments.size() && m_ColorAttachments[attachmentIndex])
        {
            const auto& spec = m_ColorAttachmentSpecifications[attachmentIndex].TextureFormat;
            GLenum base = GL_RED_INTEGER, type = GL_INT;
            Utils::GLBaseFormatType(spec, base, type);

            if (base == GL_RED_INTEGER) {
                glClearTexImage(m_ColorAttachments[attachmentIndex], 0, base, type, &value);
            }
            else {
                const GLfloat v[4] = { (float)value, 0.f, 0.f, 0.f };
                glClearTexImage(m_ColorAttachments[attachmentIndex], 0, base, GL_FLOAT, v);
            }
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        const GLint drawBuffer = (GLint)attachmentIndex;
        const GLfloat zero[4] = { (float)value, 0, 0, 0 };
        glClearBufferfv(GL_COLOR, drawBuffer, zero);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::ClearColor(float r, float g, float b, float a)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        const GLfloat color[4] = { r, g, b, a };
        const uint32_t count = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
            (uint32_t)m_ExternalColorAttachments.size());

        if (count > 0)
        {
            for (uint32_t i = 0; i < count; ++i)
                glClearBufferfv(GL_COLOR, (GLint)i, color);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::ClearDepth(float d)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        glClearBufferfv(GL_DEPTH, 0, &d);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Clear(ClearFlags flags)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        if ((uint8_t)flags & (uint8_t)ClearFlags::Color)
        {
            const GLfloat color[4] = { 0, 0, 0, 0 };
            const uint32_t count = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
                (uint32_t)m_ExternalColorAttachments.size());
            for (uint32_t i = 0; i < count; ++i)
                glClearBufferfv(GL_COLOR, (GLint)i, color);
        }
        if ((uint8_t)flags & (uint8_t)ClearFlags::Depth)
        {
            const GLfloat depth = 1.0f;
            glClearBufferfv(GL_DEPTH, 0, &depth);
        }
        if ((uint8_t)flags & (uint8_t)ClearFlags::Stencil)
        {
            const GLint stencil = 0;
            glClearBufferiv(GL_STENCIL, 0, &stencil);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::BindColorAttachment(uint32_t index) const
    {
        if (index < m_ExternalColorAttachments.size() && m_ExternalColorAttachments[index].texture)
        {
            m_ExternalColorAttachments[index].texture->Bind(0);
            return;
        }

        if (index < m_ColorAttachments.size())
        {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[index]);
        }
    }

    void OpenGLFramebuffer::SetColorAttachment(uint32_t index, const AttachmentRef& ref)
    {
        if (index >= m_ExternalColorAttachments.size())
            m_ExternalColorAttachments.resize(index + 1);
        m_ExternalColorAttachments[index] = ref;

        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        const uint32_t texID = ref.texture ? static_cast<uint32_t>(ref.texture->GetHandle()) : 0;

        if (texID == 0)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, 0, 0);
            UpdateDrawBuffers();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        bool attached = false;

        if (std::dynamic_pointer_cast<TextureCubeMap>(ref.texture))
        {
            glNamedFramebufferTextureLayer(m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
            attached = true;
        }
        else if (std::dynamic_pointer_cast<TextureArray>(ref.texture))
        {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
            attached = true;
        }
        else
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip);
            attached = true;
        }

        if (!attached)
            Q_WARNING("SetColorAttachment: unknown texture type at index %u", index);

        UpdateDrawBuffers();

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Q_WARNING("Framebuffer incomplete after SetColorAttachment (index=%u). Status=0x%X", index, status);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::shared_ptr<Texture> OpenGLFramebuffer::GetColorAttachmentTexture(uint32_t index) const
    {
        const bool multisample = m_Specification.Samples > 1;

        if (index < m_ExternalColorAttachments.size())
        {
            const auto& ref = m_ExternalColorAttachments[index];
            if (ref.texture.get()) return ref.texture;
        }

        if (!multisample)
        {
            if (index >= m_ColorAttachments.size()) return {};
            if (index >= m_ColorAttachmentViews.size())
                m_ColorAttachmentViews.resize(m_ColorAttachments.size());

            if (!m_ColorAttachmentViews[index])
            {
                TextureSpecification spec{};
                spec.width = m_Specification.Width;
                spec.height = m_Specification.Height;
                m_ColorAttachmentViews[index] =
                    std::make_shared<OpenGLTexture2DView>(m_ColorAttachments[index], GL_TEXTURE_2D, spec);
            }
            return m_ColorAttachmentViews[index];
        }

        if (index >= m_ResolvedColorTextures.size() || m_ResolvedColorTextures[index] == 0) return {};
        if (index >= m_ResolvedColorViews.size())
            m_ResolvedColorViews.resize(m_ResolvedColorTextures.size());

        if (!m_ResolvedColorViews[index])
        {
            TextureSpecification spec{};
            spec.width = m_Specification.Width;
            spec.height = m_Specification.Height;
            m_ResolvedColorViews[index] =
                std::make_shared<OpenGLTexture2DView>(m_ResolvedColorTextures[index], GL_TEXTURE_2D, spec);
        }
        return m_ResolvedColorViews[index];
    }

    void OpenGLFramebuffer::UpdateDrawBuffers() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        const uint32_t n = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
            (uint32_t)m_ExternalColorAttachments.size());

        if (n == 0)
        {
            glDrawBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        std::vector<GLenum> buffers(n, GL_NONE);
        for (uint32_t i = 0; i < n; ++i)
        {
            const bool hasInternal = (i < m_ColorAttachments.size()) && (m_ColorAttachments[i] != 0);
            const bool hasExternal = (i < m_ExternalColorAttachments.size()) && (m_ExternalColorAttachments[i].texture != nullptr);
            buffers[i] = (hasInternal || hasExternal) ? (GL_COLOR_ATTACHMENT0 + i) : GL_NONE;
        }

        glDrawBuffers((GLsizei)buffers.size(), buffers.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
