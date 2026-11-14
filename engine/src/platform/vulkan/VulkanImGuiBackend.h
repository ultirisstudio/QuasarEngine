#pragma once

#include <imgui/ImGuiBackend.h>

#include <vulkan/vulkan.h>

namespace Quasar
{
    class VulkanContext;

    class VulkanImGuiBackend final : public ImGuiBackend
    {
    public:
        bool Init(Window& window) override;
        void Shutdown() override;
        void NewFrame() override;
        void Render() override;

    private:
        VulkanContext* m_Context = nullptr;
        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    };
}
