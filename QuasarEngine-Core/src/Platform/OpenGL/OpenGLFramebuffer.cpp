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

        if (!m_ColorAttachmentSpecifications.empty())
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size(), 0);

            for (size_t i = 0; i < m_ColorAttachmentSpecifications.size(); ++i)
            {
                const auto fmt = m_ColorAttachmentSpecifications[i].TextureFormat;

                GLenum internalFormat = GL_RGBA8;
                if (fmt == FramebufferTextureFormat::RED_INTEGER)
                    internalFormat = GL_R32I;

                glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachments[i]);

                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTextureStorage2D(
                    m_ColorAttachments[i], 1, internalFormat,
                    (GLint)m_Specification.Width, (GLint)m_Specification.Height
                );

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

            if (m_Specification.DepthAsTexture)
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
                glNamedRenderbufferStorage(
                    m_DepthRBO, depthInternal,
                    (GLint)m_Specification.Width, (GLint)m_Specification.Height
                );
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
            const GLenum drawBuf = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(m_ID, 1, &drawBuf);
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
        
    }

    void* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const
    {
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
        if (attachmentIndex < m_ColorAttachments.size() && m_ColorAttachments[attachmentIndex])
        {
            const GLfloat color[4] = { r, g, b, a };
            
            for (uint32_t i = 0; i < (uint32_t)m_ColorAttachments.size(); ++i) {
                if (m_ColorAttachments[i] != 0)
                    glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)i, color);
            }
            
            for (uint32_t i = 0; i < (uint32_t)m_ExternalColorAttachments.size(); ++i) {
                if (m_ExternalColorAttachments[i].texture)
                    glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)i, color);
            }

            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        const GLint drawBuffer = (GLint)attachmentIndex;
        const GLfloat zero[4] = { r, g, b, a };
        glClearBufferfv(GL_COLOR, drawBuffer, zero);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::ClearColor(float r, float g, float b, float a)
    {
        const GLfloat color[4] = { r, g, b, a };
        const uint32_t n = std::max<uint32_t>((uint32_t)m_ColorAttachments.size(),
            (uint32_t)m_ExternalColorAttachments.size());
        for (uint32_t i = 0; i < n; ++i)
            glClearNamedFramebufferfv(m_ID, GL_COLOR, (GLint)i, color);
    }

    void OpenGLFramebuffer::ClearDepth(float d)
    {
        glClearNamedFramebufferfv(m_ID, GL_DEPTH, 0, &d);
    }

    void OpenGLFramebuffer::Clear(ClearFlags flags)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        GLbitfield mask = 0;
        if ((uint8_t)flags & (uint8_t)ClearFlags::Color)   mask |= GL_COLOR_BUFFER_BIT;
        if ((uint8_t)flags & (uint8_t)ClearFlags::Depth)   mask |= GL_DEPTH_BUFFER_BIT;
        if ((uint8_t)flags & (uint8_t)ClearFlags::Stencil) mask |= GL_STENCIL_BUFFER_BIT;

        glClear(mask);
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
            const GLenum none = GL_NONE;
            glNamedFramebufferDrawBuffers(m_ID, 1, &none);
            return;
        }

        if (std::dynamic_pointer_cast<TextureCubeMap>(ref.texture))
        {
            glNamedFramebufferTextureLayer(
                m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
        }
        else if (std::dynamic_pointer_cast<TextureArray>(ref.texture))
        {
            glNamedFramebufferTextureLayer(
                m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip, (GLint)ref.layer);
        }
        else
        {
            glNamedFramebufferTexture(
                m_ID, GL_COLOR_ATTACHMENT0 + index, texID, (GLint)ref.mip);
        }

        const GLenum buf = GL_COLOR_ATTACHMENT0 + index;
        glNamedFramebufferDrawBuffers(m_ID, 1, &buf);

        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
        glDrawBuffer(buf);

        const GLenum status = glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Q_WARNING("Framebuffer incomplete after SetColorAttachment (index=" + std::to_string(index) + "). Status=0x" + std::to_string(status));
    }

    std::shared_ptr<Texture> OpenGLFramebuffer::GetColorAttachmentTexture(uint32_t index) const
    {
        if (index < m_ExternalColorAttachments.size())
        {
            const auto& ref = m_ExternalColorAttachments[index];
            if (ref.texture.get()) return ref.texture;
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
        if (m_ColorAttachments.empty())
        {
            const GLenum none = GL_NONE;
            glNamedFramebufferDrawBuffers(m_ID, 1, &none);
            glNamedFramebufferReadBuffer(m_ID, GL_NONE);
            return;
        }

        std::vector<GLenum> buffers(m_ColorAttachments.size(), GL_NONE);
        for (uint32_t i = 0; i < (uint32_t)m_ColorAttachments.size(); ++i)
            buffers[i] = (m_ColorAttachments[i] != 0) ? (GL_COLOR_ATTACHMENT0 + i) : GL_NONE;

        glNamedFramebufferDrawBuffers(m_ID, (GLsizei)buffers.size(), buffers.data());
    }
}
