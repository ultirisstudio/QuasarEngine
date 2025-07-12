#include "qepch.h"
#include "VulkanContext.h"

#include <GLFW/glfw3.h>

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"
#include "VulkanFence.h"
#include "VulkanSemaphore.h"
#include "VulkanBuffer.h"

#include <QuasarEngine/Core/Application.h>

#include <imgui/imgui.h>

#include <vector>

namespace QuasarEngine
{
	VulkanContext::VulkanContextData VulkanContext::Context = VulkanContextData();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
	{
		std::cerr << "[VULKAN] " << callbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	VulkanContext::VulkanContext(GLFWwindow* window, bool enableValidation)
		: window(window)
	{
		Context.allocator = std::make_unique<VulkanAllocator>("Main allocator");

		Context.width = 1280;
		Context.height = 720;
		Context.imageIndex = 0;
		Context.enableValidation = enableValidation;
		Context.recreatingSwapchain = false;

		CreateInstance();
		CreateSurface();

		VkExtent2D extent = {Context.width, Context.height};

		Context.device = std::make_unique<VulkanDevice>(Context.instance, Context.surface);
		Context.swapchain = std::make_unique<VulkanSwapchain>(extent);
		Context.mainRenderPass = std::make_unique<VulkanRenderPass>(Context.device->device, 0, 0, extent.width,
		                                                            extent.height, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0,
		                                                            Context.swapchain->imageFormat,
		                                                            Context.swapchain->depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		RegenerateFramebuffer();

		CreateCommandBuffer();

		uint32_t maxFramesInFlight = Context.swapchain->maxFramesInFlight;

		for (uint8_t i = 0; i < maxFramesInFlight; ++i)
		{
			Context.frameSyncObjects.push_back({
				std::make_unique<VulkanSemaphore>(Context.device->device),
				std::make_unique<VulkanSemaphore>(Context.device->device),
				std::make_unique<VulkanFence>(Context.device->device, true)
			});
		}

		Q_DEBUG("Vulkan semaphores and fences initialized successfully");

		Context.imagesInFlight.resize(Context.swapchain->images.size());
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			Context.imagesInFlight[i] = nullptr;
		}

		Q_DEBUG("Vulkan renderer initialized successfully");
	}

	VulkanContext::~VulkanContext()
	{
		vkDeviceWaitIdle(Context.device->device);

		for (auto& cmd : Context.frameCommandBuffers)
		{
			if (cmd)
			{
				cmd->Free();
				cmd.reset();
			}
		}
		Context.frameCommandBuffers.clear();

		for (auto& fence : Context.frameFences)
		{
			if (fence)
			{
				fence.reset();
			}
		}
		Context.frameFences.clear();

		Q_DEBUG("Destroying Vulkan semaphores and fences...");
		for (uint8_t i = 0; i < Context.swapchain->maxFramesInFlight; ++i)
		{
			Context.frameSyncObjects[i].imageAvailable.reset();
			Context.frameSyncObjects[i].renderFinished.reset();
			Context.frameSyncObjects[i].inFlightFence.reset();
		}

		Context.imagesInFlight.clear();

		Q_DEBUG("Destroying Vulkan command buffers...");
		for (uint32_t i = 0; i < Context.graphicsCommandBuffer.size(); i++)
		{
			if (Context.graphicsCommandBuffer[i]->handle)
			{
				Context.graphicsCommandBuffer[i]->Free();
			}
		}

		Q_DEBUG("Destroying Vulkan framebuffers...");
		for (uint32_t i = 0; i < Context.swapchain->framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(Context.device->device, Context.swapchain->framebuffers[i], VulkanContext::Context.allocator->GetCallbacks());
		}

		Context.mainRenderPass.reset();
		Context.swapchain.reset();
		Context.device.reset();

		Q_DEBUG("Destroying Vulkan surface...");
		vkDestroySurfaceKHR(Context.instance, Context.surface, VulkanContext::Context.allocator->GetCallbacks());

		Q_DEBUG("Destroying Vulkan debugger...");
		if (Context.debugMessenger)
		{
			auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
				Context.instance, "vkDestroyDebugUtilsMessengerEXT"));
			func(Context.instance, Context.debugMessenger, VulkanContext::Context.allocator->GetCallbacks());
		}

		Q_DEBUG("Destroying Vulkan instance...");
		vkDestroyInstance(Context.instance, VulkanContext::Context.allocator->GetCallbacks());
	}

	void VulkanContext::BeginFrame()
	{
		if (Context.recreatingSwapchain)
		{
			VkResult result = vkDeviceWaitIdle(Context.device->device);
			if (!VulkanResultIsSuccess(result))
			{
				std::string str = "vkDeviceWaitIdle failed with result : " + VulkanResultString(result);
				Q_ERROR(str);
			}

			Q_DEBUG("Recreating Swapchain");
		}

		if (!Context.frameSyncObjects[Context.swapchain->currentFrame].inFlightFence->Wait(UINT64_MAX))
		{
			Q_WARNING("In-flight fence wait failure");
		}

		if (!Context.swapchain->AcquireNextImage(
			UINT64_MAX, Context.frameSyncObjects[Context.swapchain->currentFrame].imageAvailable->handle, nullptr,
			&Context.imageIndex))
		{
			Q_WARNING("Acquire next image failure");
		}

		auto& imageFence = Context.imagesInFlight[Context.imageIndex];
		if (imageFence != VK_NULL_HANDLE)
		{
			imageFence->Wait(UINT64_MAX);
		}

		Context.graphicsCommandBuffer[Context.imageIndex]->Reset();
		Context.graphicsCommandBuffer[Context.imageIndex]->Begin(false, false, false);

		float h = static_cast<float>(Context.height);
		float w = static_cast<float>(Context.width);

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = h;
		viewport.width = w;
		viewport.height = -h;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset.x = scissor.offset.y = 0;
		scissor.extent.width = Context.width;
		scissor.extent.height = Context.height;

		vkCmdSetViewport(Context.graphicsCommandBuffer[Context.imageIndex]->handle, 0, 1, &viewport);
		vkCmdSetScissor(Context.graphicsCommandBuffer[Context.imageIndex]->handle, 0, 1, &scissor);

		Context.mainRenderPass->SetWidth(Context.width);
		Context.mainRenderPass->SetHeight(Context.height);

		auto* cmdBuffer = Context.graphicsCommandBuffer[Context.imageIndex].get();
		auto* framebuffer = Context.swapchain->framebuffers[Context.imageIndex];

		Context.mainRenderPass->Begin(cmdBuffer, framebuffer);
	}

	void VulkanContext::EndFrame()
	{
		auto* cmdBuffer = Context.graphicsCommandBuffer[Context.imageIndex].get();

		Context.mainRenderPass->End(cmdBuffer);

		Context.graphicsCommandBuffer[Context.imageIndex]->End();

		if (Context.imagesInFlight[Context.imageIndex] != VK_NULL_HANDLE)
		{
			Context.imagesInFlight[Context.imageIndex]->Wait(UINT64_MAX);
		}

		Context.imagesInFlight[Context.imageIndex] = Context.frameSyncObjects[Context.swapchain->currentFrame].
		                                             inFlightFence.get();

		Context.frameSyncObjects[Context.swapchain->currentFrame].inFlightFence->Reset();

		std::vector<VkCommandBuffer> allCmdBuffers;// = Context.frameCommandBuffers;
		allCmdBuffers.push_back(Context.graphicsCommandBuffer[Context.imageIndex]->handle);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = static_cast<uint32_t>(allCmdBuffers.size());
		submit_info.pCommandBuffers = allCmdBuffers.data();

		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &Context.frameSyncObjects[Context.swapchain->currentFrame].renderFinished->handle;

		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &Context.frameSyncObjects[Context.swapchain->currentFrame].imageAvailable->handle;

		VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.pWaitDstStageMask = flags;

		VkResult result = vkQueueSubmit(Context.device->graphicsQueue, 1, &submit_info,
		                                Context.frameSyncObjects[Context.swapchain->currentFrame].inFlightFence->
		                                handle);
		if (result != VK_SUCCESS)
		{
			std::string str = "vkQueueSubmit failed with result : " + VulkanResultString(result);
			Q_ERROR(str);
		}

		Context.graphicsCommandBuffer[Context.imageIndex]->UpdateSubmitted();

		Context.swapchain->PresentImage(Context.imageIndex,
		                                Context.frameSyncObjects[Context.swapchain->currentFrame].renderFinished->
		                                handle);
	}

	void VulkanContext::Resize(unsigned int newWidth, unsigned int newHeight)
	{
		Context.width = newWidth;
		Context.height = newHeight;

		std::string str = std::to_string(Context.width) + " " + std::to_string(Context.height);
		Q_DEBUG(str);

		RegenerateSwapchain();
	}

	void VulkanContext::BeginSingleTimeCommands()
	{
		if (!Context.frameFences.empty() && Context.frameFences.back())
		{
			Context.frameFences.back()->Wait(UINT64_MAX);
		}

		std::unique_ptr<VulkanFence> fence = std::make_unique<VulkanFence>(Context.device->device, true);

		Context.frameFences.push_back(std::move(fence));

		auto& cmd = Context.frameCommandBuffers.emplace_back();
		cmd = std::make_unique<VulkanCommandBuffer>(Context.device->device, Context.device->graphicsCommandPool);
		cmd->Allocate(true);
		cmd->Begin(true, false, false);
	}

	void VulkanContext::EndSingleTimeCommands()
	{
		auto& commandBuffer = Context.frameCommandBuffers.back();

		if (commandBuffer)
		{
			commandBuffer->End();

			auto& fence = Context.frameFences.back();
			if (fence)
			{
				fence->Reset();

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer->handle;

				VK_CHECK(vkQueueSubmit(Context.device->graphicsQueue, 1, &submitInfo, fence->handle));

				fence->Wait(UINT64_MAX);

				fence.reset();
				Context.frameFences.pop_back();
			}

			commandBuffer.reset();
			Context.frameCommandBuffers.pop_back();
		}
	}

	void VulkanContext::CreateInstance()
	{
		if (Context.enableValidation && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app_info.apiVersion = VK_API_VERSION_1_3;
		app_info.pApplicationName = "Quasar Engine";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Quasar Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

		auto extensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &app_info;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (Context.enableValidation)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(Context.validationLayers.size());
			createInfo.ppEnabledLayerNames = Context.validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

#ifdef DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = DebugCallback;

		createInfo.pNext = &debugCreateInfo;
#else
		createInfo.pNext = nullptr;
#endif

		VK_CHECK(vkCreateInstance(&createInfo, VulkanContext::Context.allocator->GetCallbacks(), &VulkanContext::Context.instance));

		Q_DEBUG("Vulkan instance created successfully");

		auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
			Context.instance, "vkCreateDebugUtilsMessengerEXT"));

		Q_ASSERT(func != nullptr, "Failed to create debug messenger");

#ifdef DEBUG
		VK_CHECK(
			func(VulkanContext::Context.instance, &debugCreateInfo, VulkanContext::Context.allocator->GetCallbacks(), &VulkanContext::Context.debugMessenger));
#endif

		Q_DEBUG("Vulkan debug messenger created successfully");
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK(
			glfwCreateWindowSurface(VulkanContext::Context.instance, window, VulkanContext::Context.allocator->GetCallbacks(), &VulkanContext::Context.surface));

		Q_DEBUG("Window surface created successfully");
	}

	void VulkanContext::CreateCommandBuffer()
	{
		Context.graphicsCommandBuffer.clear();
		Context.graphicsCommandBuffer.reserve(Context.swapchain->views.size());
		for (uint32_t i = 0; i < Context.swapchain->views.size(); i++)
		{
			Context.graphicsCommandBuffer.push_back(
				std::make_unique<VulkanCommandBuffer>(Context.device->device, Context.device->graphicsCommandPool));
		}

		for (uint32_t i = 0; i < Context.swapchain->views.size(); i++)
		{
			if (Context.graphicsCommandBuffer[i]->handle)
			{
				Context.graphicsCommandBuffer[i]->Free();
			}

			Context.graphicsCommandBuffer[i]->Allocate(true);
		}

		Q_DEBUG("Vulkan command buffers created successfully");
	}

	void VulkanContext::RegenerateFramebuffer()
	{
		for (uint32_t i = 0; i < Context.swapchain->views.size(); i++)
		{
			std::vector<VkImageView> attachments;
			attachments.push_back(Context.swapchain->views[i]);
			attachments.push_back(Context.swapchain->depthAttachment->view);

			VkExtent2D extent = {Context.width, Context.height};

			Context.swapchain->framebuffers.push_back(VkFramebuffer());

			VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
			framebuffer_create_info.renderPass = Context.mainRenderPass->renderpass;
			framebuffer_create_info.attachmentCount = attachments.size();
			framebuffer_create_info.pAttachments = attachments.data();
			framebuffer_create_info.width = extent.width;
			framebuffer_create_info.height = extent.height;
			framebuffer_create_info.layers = 1;

			VK_CHECK(
				vkCreateFramebuffer(VulkanContext::Context.device->device, &framebuffer_create_info, VulkanContext::Context.allocator->GetCallbacks(), &
					VulkanContext::Context.swapchain->framebuffers.back()));

			Q_DEBUG("Vulkan framebuffer created successfully");
		}
	}

	void VulkanContext::RegenerateSwapchain()
	{
		if (Context.recreatingSwapchain)
		{
			Q_DEBUG("Swapchain is already recreating");
			return;
		}

		if (Context.width == 0 || Context.height == 0)
		{
			Q_DEBUG("Wrong dimension size");
		}

		Context.recreatingSwapchain = true;

		vkDeviceWaitIdle(Context.device->device);

		for (uint32_t i = 0; i < Context.swapchain->images.size(); i++)
		{
			Context.imagesInFlight[i] = nullptr;
		}

		VkExtent2D extent = {Context.width, Context.height};
		Context.swapchain->RecreateSwapchain(extent);

		Context.mainRenderPass->SetWidth(Context.width);
		Context.mainRenderPass->SetHeight(Context.height);

		for (uint32_t i = 0; i < Context.swapchain->images.size(); i++)
		{
			Context.graphicsCommandBuffer[i]->Free();
		}

		Context.graphicsCommandBuffer.clear();

		for (uint32_t i = 0; i < Context.swapchain->images.size(); i++)
		{
			vkDestroyFramebuffer(Context.device->device, Context.swapchain->framebuffers[i], VulkanContext::Context.allocator->GetCallbacks());
		}

		Context.swapchain->framebuffers.clear();

		Context.mainRenderPass->SetX(0);
		Context.mainRenderPass->SetY(0);
		Context.mainRenderPass->SetWidth(Context.width);
		Context.mainRenderPass->SetHeight(Context.height);

		RegenerateFramebuffer();

		CreateCommandBuffer();

		Context.recreatingSwapchain = false;
	}

	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : Context.validationLayers)
		{
			bool found = false;
			for (const auto& layerProps : availableLayers)
			{
				if (strcmp(layerName, layerProps.layerName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found) return false;
		}
		return true;
	}

	std::vector<const char*> VulkanContext::GetRequiredExtensions()
	{
		uint32_t count;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
		if (Context.enableValidation)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}
}
