#include <platform/vulkan/VulkanImGuiBackend.h>

#include <quasar/core/Window.h>
#include <quasar/log/Log.h>
#include <quasar/platform/vulkan/VulkanContext.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>

namespace Quasar
{
    namespace
    {
        void CheckResult(VkResult result)
        {
            if (result < 0)
            {
                QUASAR_ENGINE_ERROR("The Vulkan backend of ImGui got the Vulkan error {}", static_cast<int>(result));
            }
        }
    }

    bool VulkanImGuiBackend::Init(Window& window)
    {
        m_Context = dynamic_cast<VulkanContext*>(window.GetGraphicsContext());
        if (m_Context == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot start the Vulkan backend of ImGui: the window has no Vulkan context");
            return false;
        }

        if (!ImGui_ImplSDL3_InitForVulkan(window.GetNativeWindow()))
        {
            QUASAR_ENGINE_ERROR("Cannot start the SDL backend of ImGui");
            return false;
        }

        m_ImageFormat = m_Context->GetImageFormat();

        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_3;
        info.Instance = m_Context->GetInstance();
        info.PhysicalDevice = m_Context->GetPhysicalDevice();
        info.Device = m_Context->GetDevice();
        info.QueueFamily = m_Context->GetQueueFamily();
        info.Queue = m_Context->GetQueue();
        info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
        info.MinImageCount = VulkanContext::FramesInFlight;
        info.ImageCount = std::max(m_Context->GetImageCount(), VulkanContext::FramesInFlight);
        info.UseDynamicRendering = true;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ImageFormat;
        info.CheckVkResultFn = CheckResult;

        if (!ImGui_ImplVulkan_Init(&info))
        {
            QUASAR_ENGINE_ERROR("Cannot start the Vulkan backend of ImGui");
            ImGui_ImplSDL3_Shutdown();
            return false;
        }
        return true;
    }

    void VulkanImGuiBackend::Shutdown()
    {
        vkDeviceWaitIdle(m_Context->GetDevice());
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }

    void VulkanImGuiBackend::NewFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }

    void VulkanImGuiBackend::Render()
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Context->GetCommandBuffer());

        if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}
