#pragma once

#include <QuasarEngine/Renderer/Framebuffer.h>
#include <vector>
#include <memory>

namespace QuasarEngine
{
    class Texture;

    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        ~OpenGLFramebuffer();

        OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer& operator=(const OpenGLFramebuffer&) = delete;

        void Bind() const override;
        void Unbind() const override;

        void Resize(uint32_t width, uint32_t height) override;
        void Invalidate() override;
        void Resolve() override;

        int  ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        void ClearAttachment(uint32_t attachmentIndex, int value) override;
        void ClearColor(float r, float g, float b, float a) override;
        void ClearDepth(float d = 1.0f) override;
        void Clear(ClearFlags flags = ClearFlags::All) override;

        void BindColorAttachment(uint32_t index = 0) const override;

        void SetColorAttachment(uint32_t index, const AttachmentRef& ref) override;

        void* GetColorAttachment(uint32_t index) const override;
        void* GetDepthAttachment() const override;

        std::shared_ptr<Texture> GetColorAttachmentTexture(uint32_t index) const override;

    private:
        void UpdateDrawBuffers() const;

        void DestroyInternalTargets();
        void CreateResolveTargetsIfNeeded();
        void DestroyResolveTargets();

    private:
        uint32_t m_ID = 0;

        std::vector<uint32_t> m_ColorAttachments;
        
        uint32_t m_DepthRBO = 0;
        uint32_t m_DepthTexture = 0;
        bool     m_DepthIsTexture = false;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification;

        std::vector<AttachmentRef> m_ExternalColorAttachments;

        std::vector<uint32_t> m_ResolveFBOs;
        std::vector<uint32_t> m_ResolvedColorTextures;

        mutable std::vector<std::shared_ptr<Texture>> m_ColorAttachmentViews;
        mutable std::vector<std::shared_ptr<Texture>> m_ResolvedColorViews;
    };
}
