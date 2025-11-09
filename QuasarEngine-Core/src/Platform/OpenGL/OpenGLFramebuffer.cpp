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
        static bool IsDepthFormat(FramebufferTextureFormat format)
        {
            switch (format)
            {
            case FramebufferTextureFormat::DEPTH24:
            case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
            default: return false;
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

        if (m_DepthRBO) { glDeleteRenderbuffers(1, &m_DepthRBO);   m_DepthRBO = 0; }
        if (m_DepthTexture) { glDeleteTextures(1, &m_DepthTexture);    m_DepthTexture = 0; }
        m_DepthIsTexture = false;
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
        DestroyResolveTargets();

        if (m_Specification.Samples <= 1)
            return;

        m_ResolveFBOs.resize(m_ColorAttachments.size(), 0);
        m_ResolvedColorTextures.resize(m_ColorAttachments.size(), 0);

        for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
        {
            if (m_ColorAttachments[i] == 0)
                continue;

            glCreateTextures(GL_TEXTURE_2D, 1, &m_ResolvedColorTextures[i]);
            glTextureParameteri(m_ResolvedColorTextures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_ResolvedColorTextures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_ResolvedColorTextures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_ResolvedColorTextures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GLenum internalFormat = GL_RGBA8;
            if (i < m_ColorAttachmentSpecifications.size() &&
                m_ColorAttachmentSpecifications[i].TextureFormat == FramebufferTextureFormat::RED_INTEGER)
                internalFormat = GL_R32I;

            glTextureStorage2D(m_ResolvedColorTextures[i], 1, internalFormat,
                (GLint)m_Specification.Width, (GLint)m_Specification.Height);

            glCreateFramebuffers(1, &m_ResolveFBOs[i]);
            glNamedFramebufferTexture(m_ResolveFBOs[i], GL_COLOR_ATTACHMENT0, m_ResolvedColorTextures[i], 0);
            const GLenum draw = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_ResolveFBOs[i], 1, &draw);

            const GLenum status = glCheckNamedFramebufferStatus(m_ResolveFBOs[i], GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                Q_WARNING("Resolve FBO incomplete (i=" + std::to_string(i) + "). Status=0x" + std::to_string(status));
        }
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

        Invalidate();
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

        if (!m_ColorAttachmentSpecifications.empty())
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size(), 0);

            for (size_t i = 0; i < m_ColorAttachmentSpecifications.size(); ++i)
            {
                const auto fmt = m_ColorAttachmentSpecifications[i].TextureFormat;

                const bool isInteger = (fmt == FramebufferTextureFormat::RED_INTEGER);
                const GLenum internalFormat = isInteger ? GL_R32I : GL_RGBA8;

                const bool msaa = (m_Specification.Samples > 1);
                const GLenum target = msaa ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

                glCreateTextures(target, 1, &m_ColorAttachments[i]);

                if (msaa)
                {
                    glTextureStorage2DMultisample(
                        m_ColorAttachments[i],
                        (GLint)m_Specification.Samples,
                        internalFormat,
                        (GLint)m_Specification.Width,
                        (GLint)m_Specification.Height,
                        GL_FALSE
                    );
                }
                else
                {
                    glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    glTextureStorage2D(
                        m_ColorAttachments[i], 1, internalFormat,
                        (GLint)m_Specification.Width, (GLint)m_Specification.Height
                    );
                }

                glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0 + (GLenum)i, m_ColorAttachments[i], 0);
            }
        }

        if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
        {
            if (m_DepthRBO) { glDeleteRenderbuffers(1, &m_DepthRBO);   m_DepthRBO = 0; }
            if (m_DepthTexture) { glDeleteTextures(1, &m_DepthTexture);    m_DepthTexture = 0; }
            m_DepthIsTexture = false;

            const bool wantStencil =
                (m_DepthAttachmentSpecification.TextureFormat == FramebufferTextureFormat::DEPTH24STENCIL8);
            const GLenum depthInternal = wantStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24;
            const GLenum depthAttach = wantStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

            const bool msaa = (m_Specification.Samples > 1);

            if (m_Specification.DepthAsTexture && !msaa)
            {
                m_DepthIsTexture = true;
                glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthTexture);
                glTextureParameteri(m_DepthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTextureParameteri(m_DepthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTextureStorage2D(
                    m_DepthTexture, 1, depthInternal,
                    (GLint)m_Specification.Width, (GLint)m_Specification.Height
                );
                glNamedFramebufferTexture(m_ID, depthAttach, m_DepthTexture, 0);
            }
            else
            {
                glCreateRenderbuffers(1, &m_DepthRBO);
                if (msaa)
                {
                    glNamedRenderbufferStorageMultisample(
                        m_DepthRBO, (GLint)m_Specification.Samples, depthInternal,
                        (GLint)m_Specification.Width, (GLint)m_Specification.Height
                    );
                }
                else
                {
                    glNamedRenderbufferStorage(
                        m_DepthRBO, depthInternal,
                        (GLint)m_Specification.Width, (GLint)m_Specification.Height
                    );
                }
                glNamedFramebufferRenderbuffer(m_ID, depthAttach, GL_RENDERBUFFER, m_DepthRBO);
            }
        }

        if (!m_ColorAttachments.empty())
        {
            std::vector<GLenum> bufs(m_ColorAttachments.size(), GL_NONE);
            for (uint32_t i = 0; i < (uint32_t)m_ColorAttachments.size(); ++i)
                bufs[i] = (m_ColorAttachments[i] != 0) ? (GL_COLOR_ATTACHMENT0 + i) : GL_NONE;
            glNamedFramebufferDrawBuffers(m_ID, (GLsizei)bufs.size(), bufs.data());
        }
        else
        {
            const GLenum none = GL_NONE;
            glNamedFramebufferDrawBuffers(m_ID, 1, &none);
            glNamedFramebufferReadBuffer(m_ID, GL_NONE);
        }

        CreateResolveTargetsIfNeeded();

        const GLenum status = glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Q_ERROR("Framebuffer incomplete! status=0x" + std::to_string(status));

        m_ColorAttachmentViews.clear();
        m_ResolvedColorViews.clear();
    }

    void OpenGLFramebuffer::Resolve()
    {
        if (m_Specification.Samples <= 1)
            return;

        for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
        {
            if (i >= m_ResolveFBOs.size() || m_ResolveFBOs[i] == 0)
                continue;

            glNamedFramebufferReadBuffer(m_ID, GL_COLOR_ATTACHMENT0 + (GLenum)i);
            const GLenum draw = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_ResolveFBOs[i], 1, &draw);

            glBlitNamedFramebuffer(
                m_ID, m_ResolveFBOs[i],
                0, 0, (GLint)m_Specification.Width, (GLint)m_Specification.Height,
                0, 0, (GLint)m_Specification.Width, (GLint)m_Specification.Height,
                GL_COLOR_BUFFER_BIT, GL_NEAREST
            );
        }
    }

    void* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (m_Specification.Samples > 1)
        {
            if (index < m_ResolvedColorTextures.size() && m_ResolvedColorTextures[index] != 0)
                return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ResolvedColorTextures[index]));
            return nullptr;
        }

        if (index < m_ColorAttachments.size())
            return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ColorAttachments[index]));

        if (index < m_ExternalColorAttachments.size())
        {
            const auto& ar = m_ExternalColorAttachments[index];
            if (ar.texture)
            {
                const auto id = static_cast<uint32_t>(ar.texture->GetHandle());
                return reinterpret_cast<void*>(static_cast<uintptr_t>(id));
            }
        }
        return nullptr;
    }

    void* OpenGLFramebuffer::GetDepthAttachment() const
    {
        if (m_DepthIsTexture)
            return reinterpret_cast<void*>(static_cast<uintptr_t>(m_DepthTexture));
        else
            return reinterpret_cast<void*>(static_cast<uintptr_t>(m_DepthRBO));
    }

    int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        GLenum base = GL_RED_INTEGER, type = GL_INT;
        if (attachmentIndex < m_ColorAttachmentSpecifications.size()) {
            Utils::GLBaseFormatType(m_ColorAttachmentSpecifications[attachmentIndex].TextureFormat, base, type);
        }

        int out = 0;
        if (base == GL_RED_INTEGER) {
            glReadPixels(x, y, 1, 1, base, type, &out);
        }
        else {
            if (type == GL_FLOAT) {
                float pix[4] = { 0 };
                glReadPixels(x, y, 1, 1, base, GL_FLOAT, pix);
                out = (int)pix[0];
            }
            else {
                unsigned char pix[4] = { 0 };
                glReadPixels(x, y, 1, 1, base, GL_UNSIGNED_BYTE, pix);
                out = (int)pix[0];
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return out;
    }

    void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, float r, float g, float b, float a)
    {
        const bool isInteger =
            (attachmentIndex < m_ColorAttachmentSpecifications.size()) &&
            (m_ColorAttachmentSpecifications[attachmentIndex].TextureFormat == FramebufferTextureFormat::RED_INTEGER);

        if (isInteger)
        {
            const GLint icolor[4] = { (GLint)r, (GLint)g, (GLint)b, (GLint)a };
            glClearNamedFramebufferiv(m_ID, GL_COLOR, (GLint)attachmentIndex, icolor);
            return;
        }
        else
        {
            const GLfloat color[4] = { r, g, b, a };
            glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)attachmentIndex, color);
            return;
        }
    }

    void OpenGLFramebuffer::ClearColor(float r, float g, float b, float a)
    {
        m_LastClearColor[0] = r; m_LastClearColor[1] = g;
        m_LastClearColor[2] = b; m_LastClearColor[3] = a;

        const uint32_t n = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
            (uint32_t)m_ExternalColorAttachments.size());
        for (uint32_t i = 0; i < n; ++i)
            glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)i, m_LastClearColor);
    }


    void OpenGLFramebuffer::ClearDepth(float d)
    {
        m_LastClearDepth = d;
        glClearNamedFramebufferfv(m_ID, GL_DEPTH, 0, &m_LastClearDepth);
    }

    void OpenGLFramebuffer::Clear(ClearFlags flags)
    {
        if ((uint8_t)flags & (uint8_t)ClearFlags::Color)
        {
            const uint32_t n = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
                (uint32_t)m_ExternalColorAttachments.size());
            for (uint32_t i = 0; i < n; ++i)
                glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)i, m_LastClearColor);
        }

        const bool wantDepth = ((uint8_t)flags & (uint8_t)ClearFlags::Depth) != 0;
        const bool wantStencil = ((uint8_t)flags & (uint8_t)ClearFlags::Stencil) != 0;

        if (wantDepth || wantStencil)
        {
            const bool hasDS =
                (m_DepthAttachmentSpecification.TextureFormat == FramebufferTextureFormat::DEPTH24STENCIL8);

            if (wantDepth && wantStencil && hasDS)
            {
                glClearNamedFramebufferfi(m_ID, GL_DEPTH_STENCIL, 0, m_LastClearDepth, m_LastClearStencil);
            }
            else
            {
                if (wantDepth)
                    glClearNamedFramebufferfv(m_ID, GL_DEPTH, 0, &m_LastClearDepth);
                if (wantStencil)
                    glClearNamedFramebufferiv(m_ID, GL_STENCIL, 0, &m_LastClearStencil);
            }
        }
    }

    void OpenGLFramebuffer::BindColorAttachment(uint32_t index) const
    {
        if (index < m_ExternalColorAttachments.size() && m_ExternalColorAttachments[index].texture)
        {
            m_ExternalColorAttachments[index].texture->Bind(0);
            return;
        }

        if (index < m_ColorAttachments.size()) {
            glBindTextureUnit(0, m_ColorAttachments[index]);
        }
    }

    void OpenGLFramebuffer::SetColorAttachment(uint32_t index, const AttachmentRef& ref)
    {
        if (index >= m_ExternalColorAttachments.size())
            m_ExternalColorAttachments.resize(index + 1);
        m_ExternalColorAttachments[index] = ref;

        const GLuint texID = ref.texture ? (GLuint)ref.texture->GetHandle() : 0;

        if (texID == 0)
        {
            glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0 + index, 0, 0);

            UpdateDrawBuffers();

            return;
        }

        if (std::dynamic_pointer_cast<TextureCubeMap>(ref.texture))
        {
            glNamedFramebufferTextureLayer(m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
        }
        else if (std::dynamic_pointer_cast<TextureArray>(ref.texture))
        {
            glNamedFramebufferTextureLayer(m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
        }
        else
        {
            glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip);
        }

        UpdateDrawBuffers();

        const GLenum status = glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Q_WARNING("Framebuffer incomplete after SetColorAttachment (index=" + std::to_string(index) + "). Status=0x" + std::to_string(status));
    }

    std::shared_ptr<Texture> OpenGLFramebuffer::GetColorAttachmentTexture(uint32_t index) const
    {
        if (m_Specification.Samples > 1)
        {
            if (index >= m_ResolvedColorTextures.size()) return {};
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

    void OpenGLFramebuffer::UpdateDrawBuffers() const
    {
        const uint32_t nInternal = (uint32_t)m_ColorAttachments.size();
        const uint32_t nExternal = (uint32_t)m_ExternalColorAttachments.size();
        const uint32_t n = std::max(nInternal, nExternal);

        if (n == 0)
        {
            const GLenum none = GL_NONE;
            glNamedFramebufferDrawBuffers(m_ID, 1, &none);
            glNamedFramebufferReadBuffer(m_ID, GL_NONE);
            return;
        }

        std::vector<GLenum> buffers(n, GL_NONE);
        for (uint32_t i = 0; i < n; ++i)
        {
            const bool hasInternal = (i < nInternal) && (m_ColorAttachments[i] != 0);
            const bool hasExternal = (i < nExternal) && (m_ExternalColorAttachments[i].texture != nullptr);
            buffers[i] = (hasInternal || hasExternal) ? (GL_COLOR_ATTACHMENT0 + i) : GL_NONE;
        }

        glNamedFramebufferDrawBuffers(m_ID, (GLsizei)buffers.size(), buffers.data());
    }
}
