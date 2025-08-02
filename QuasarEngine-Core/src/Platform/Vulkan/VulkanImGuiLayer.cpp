#include "qepch.h"
#include "VulkanImGuiLayer.h"

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <ImGuizmo.h>

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include <QuasarEngine/Core/Application.h>
#include <Platform/Vulkan/VulkanContext.h>
#include <Platform/Vulkan/VulkanDevice.h>
#include <Platform/Vulkan/VulkanSwapchain.h>
#include <Platform/Vulkan/VulkanRenderpass.h>
#include <Platform/Vulkan/VulkanCommandBuffer.h>

namespace QuasarEngine
{
	VulkanImGuiLayer::VulkanImGuiLayer() : ImGuiLayer("ImGuiLayer"), descriptorPool(VK_NULL_HANDLE)
	{
	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
		
	}

	void VulkanImGuiLayer::OnAttach()
	{
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        float fontSize = 14.0f;
        io.Fonts->AddFontFromFileTTF("Assets/Fonts/Monocraft.ttf", fontSize);
        io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/Monocraft.ttf", fontSize);

        /*static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF("Assets/Fonts/Font-Awesome-6-Free-Solid-900.otf", fontSize, &icons_config, icons_ranges);*/

        io.Fonts->Build();

        //ImGui::StyleColorsDark();
        ImGui::StyleColorsRealDark();
        //ImGui::StyleColorsModernDark();
        //ImGui::StyleColorsModernDarkBis();
        //ImGui::StyleColorsModernLight();
        //ImGui::StyleColorsLight();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
        ImGui_ImplGlfw_InitForVulkan(window, true);

        auto& context = VulkanContext::Context;
        auto* device = context.device.get();

        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
        poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;
        vkCreateDescriptorPool(VulkanContext::Context.device->device, &poolInfo, VulkanContext::Context.allocator->GetCallbacks(), &descriptorPool);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = context.instance;
        initInfo.PhysicalDevice = device->physicalDevice;
        initInfo.Device = device->device;
        initInfo.QueueFamily = device->GetQueueFamilyIndices().graphicsFamily.value();
        initInfo.Queue = device->graphicsQueue;
        initInfo.RenderPass = context.mainRenderPass->renderpass;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = descriptorPool;
        initInfo.MinImageCount = static_cast<uint32_t>(context.swapchain->images.size());
        initInfo.ImageCount = static_cast<uint32_t>(context.swapchain->images.size());
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator = VulkanContext::Context.allocator->GetCallbacks();
        initInfo.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) {
                Q_ERROR("Vulkan error: {}", err);
            }
        };

        ImGui_ImplVulkan_Init(&initInfo);
	}

	void VulkanImGuiLayer::OnDetach()
	{
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        auto* device = VulkanContext::Context.device.get();
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device->device, descriptorPool, VulkanContext::Context.allocator->GetCallbacks());
            descriptorPool = VK_NULL_HANDLE;
        }
	}

	void VulkanImGuiLayer::Begin()
	{
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
	}

	void VulkanImGuiLayer::End()
	{
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VulkanContext::Context.graphicsCommandBuffer[VulkanContext::Context.imageIndex]->handle);

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
	}
}