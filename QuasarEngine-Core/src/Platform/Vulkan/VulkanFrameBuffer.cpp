#include "qepch.h"

#include "VulkanTypes.h"
#include "VulkanFramebuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

#include "QuasarEngine/Renderer/RenderCommand.h"

namespace QuasarEngine
{
    VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
        : Framebuffer(spec)
    {
        
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        Cleanup();
    }

    void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        m_Specification.Width = width;
        m_Specification.Height = height;
        Invalidate();
    }

    void VulkanFramebuffer::Invalidate()
    {
        Cleanup();
        CreateAttachments();
        CreateDescriptorSets();
    }

    void VulkanFramebuffer::Resolve()
    {

    }

    void VulkanFramebuffer::CreateAttachments()
    {
        const auto& device = VulkanContext::Context.device->device;
        const auto& attachments = m_Specification.Attachments.Attachments;
        uint32_t width = m_Specification.Width;
        uint32_t height = m_Specification.Height;

        m_Attachments.clear();
        m_Attachments.reserve(attachments.size());

        std::vector<VkAttachmentDescription> vkAttachmentDescs;
        std::vector<VkAttachmentReference> colorRefs;
        VkAttachmentReference depthRef = {};

        for (size_t i = 0; i < attachments.size(); ++i)
        {
            auto format = attachments[i].TextureFormat;
            VkFormat vkFormat;
            VkImageAspectFlags aspect = 0;
            bool isDepth = false;

            switch (format)
            {
            case FramebufferTextureFormat::RGBA8:
                //vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
                vkFormat = VulkanContext::Context.swapchain->imageFormat.format;
                aspect = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            case FramebufferTextureFormat::RED_INTEGER:
                vkFormat = VK_FORMAT_R32_SINT;
                aspect = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            case FramebufferTextureFormat::DEPTH24STENCIL8:
                //vkFormat = VK_FORMAT_D24_UNORM_S8_UINT;
                vkFormat = VulkanContext::Context.swapchain->depthFormat;
                aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
                isDepth = true;
                break;
            default:
                throw std::runtime_error("Unsupported framebuffer texture format");
            }

            auto image = std::make_unique<VulkanImage>(
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_VIEW_TYPE_2D,
                width,
                height,
                vkFormat,
                VK_IMAGE_TILING_OPTIMAL,
                isDepth ?
                (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                : (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                true,
                aspect
            );
            m_Attachments.push_back(std::move(image));

            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format = vkFormat;
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkAttachmentDescs.push_back(attachmentDesc);

            if (isDepth)
                depthRef = { static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
            else
                colorRefs.push_back({ static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        }

        m_RenderPass = std::make_unique<VulkanRenderPass>(
            device,
            0.0f, 0.0f,
            static_cast<float>(width),
            static_cast<float>(height),
            0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 0,
            VulkanContext::Context.swapchain->imageFormat,
            VulkanContext::Context.swapchain->depthFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        std::vector<VkImageView> views;
        for (auto& img : m_Attachments)
            views.push_back(img->view);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass->renderpass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
        framebufferInfo.pAttachments = views.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffer));

        if (!m_Sampler)
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));
        }
    }

    void VulkanFramebuffer::CreateDescriptorSets()
    {
        const auto& device = VulkanContext::Context.device->device;

        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_Attachments.size()) }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = static_cast<uint32_t>(m_Attachments.size());

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool));

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

        m_DescriptorSets.resize(m_Attachments.size());
        std::vector<VkDescriptorSetLayout> layouts(m_Attachments.size(), layout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));

        for (size_t i = 0; i < m_Attachments.size(); ++i)
        {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = m_Attachments[i]->view;
            imgInfo.sampler = m_Sampler;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_DescriptorSets[i];
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imgInfo;

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

    void VulkanFramebuffer::Bind() const
    {
        VulkanContext::BeginSingleTimeCommands();

        VkClearValue clearValues[2];
        clearValues[0].color = { {0.0f, 0.0f, 0.2f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = m_RenderPass->renderpass;
        info.framebuffer = m_Framebuffer;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { m_Specification.Width, m_Specification.Height };
        info.clearValueCount = 2;
        info.pClearValues = clearValues;

        RenderCommand::SetViewport(0, 0, m_Specification.Width, m_Specification.Height);

        vkCmdBeginRenderPass(VulkanContext::Context.frameCommandBuffers.back()->handle, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanFramebuffer::Unbind() const
    {
        vkCmdEndRenderPass(VulkanContext::Context.frameCommandBuffers.back()->handle);

		VulkanContext::EndSingleTimeCommands();

        /*for (const auto& img : m_Attachments)
        {
            img->ImageTransitionLayout(cmdBuffer,
                img->GetFormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }*/
    }

    void VulkanFramebuffer::Cleanup()
    {
        const auto& device = VulkanContext::Context.device->device;

        vkDeviceWaitIdle(device);

        if (m_Framebuffer)
        {
            vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
            m_Framebuffer = VK_NULL_HANDLE;
        }
        if (m_DescriptorPool)
        {
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        if (m_Sampler)
        {
            vkDestroySampler(device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }
        m_RenderPass.reset();
        m_Attachments.clear();
        m_DescriptorSets.clear();
    }

    void VulkanFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
    {

    }

    int VulkanFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        return 0;
    }

    void* VulkanFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_DescriptorSets.size()) return nullptr;
        return reinterpret_cast<void*>(m_DescriptorSets[index]);
    }

    void* VulkanFramebuffer::GetDepthAttachment() const
    {
        return nullptr;
    }
}