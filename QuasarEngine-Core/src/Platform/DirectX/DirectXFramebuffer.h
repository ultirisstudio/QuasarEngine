#pragma once

#include <QuasarEngine/Renderer/Framebuffer.h>

namespace QuasarEngine
{
	class DirectXFramebuffer : public Framebuffer
	{
    public:
        DirectXFramebuffer(const FramebufferSpecification& spec);
        ~DirectXFramebuffer();

        DirectXFramebuffer(const DirectXFramebuffer&) = delete;
        DirectXFramebuffer& operator=(const DirectXFramebuffer&) = delete;

        void* GetColorAttachment(uint32_t index) const override;
        void* GetDepthAttachment() const override;

        int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        void ClearAttachment(uint32_t attachmentIndex, int value) override;

        void Resize(uint32_t width, uint32_t height) override;
        void Invalidate() override;

        void Resolve() override;

        void Bind() const override;
        void Unbind() const override;

        void BindColorAttachment(uint32_t index = 0) const override;

    private:
        uint32_t m_ID;
        FramebufferSpecification m_Specification;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification;

        std::vector<uint32_t> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0;
        
        uint32_t m_ResolveFBO = 0;
        uint32_t m_ResolvedColorTexture = 0;
	};
}