#pragma once

#include <vulkan/vulkan.h>

#include <QuasarEngine/Renderer/Framebuffer.h>

namespace QuasarEngine
{
    class VulkanImage;
    class VulkanRenderPass;

    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferSpecification& spec);
        ~VulkanFramebuffer() override;

        VulkanFramebuffer(const VulkanFramebuffer&) = delete;
        VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;

        void Resize(uint32_t width, uint32_t height) override;
        void Invalidate() override;

        void Resolve() override;

        void Bind() const override;
        void Unbind() const override;

        void ClearAttachment(uint32_t attachmentIndex, int value) override;
        int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        void* GetColorAttachment(uint32_t index) const override;
        void* GetDepthAttachment() const override;

        void BindColorAttachment(uint32_t index = 0) const override {}

        VulkanRenderPass* GetRenderPass() const { return m_RenderPass.get(); }

        VkFramebuffer GetVkFramebuffer() const { return m_Framebuffer; }
        const std::vector<std::unique_ptr<VulkanImage>>& GetAttachments() const { return m_Attachments; }

    private:
        void Cleanup();
        void CreateAttachments();
        void CreateDescriptorSets();

        std::vector<std::unique_ptr<VulkanImage>> m_Attachments;
        std::vector<VkDescriptorSet> m_DescriptorSets;

        std::unique_ptr<VulkanRenderPass> m_RenderPass;

        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}