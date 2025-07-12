#pragma once

#include <vulkan/vulkan.h>

#include "VulkanAllocator.h"

#include <QuasarEngine/Renderer/GraphicsContext.h>

#include <memory>

struct GLFWwindow;

namespace QuasarEngine
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanRenderPass;
	class VulkanCommandBuffer;
	class VulkanFence;
	class VulkanSemaphore;
	class VulkanShader;
	class VulkanBuffer;
	class VulkanVertexBuffer;
	class VulkanIndexBuffer;

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* window, bool enableValidation = true);
		~VulkanContext() override;

		VulkanContext(const VulkanContext&) = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;

		void BeginFrame() override;
		void EndFrame() override;

		void Resize(unsigned int newWidth, unsigned int newHeight) override;

		static void BeginSingleTimeCommands();
		static void EndSingleTimeCommands();

	private:
		void CreateInstance();
		void CreateSurface();
		void CreateCommandBuffer();
		void RegenerateFramebuffer();
		void RegenerateSwapchain();

		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();

		GLFWwindow* window;

		struct VulkanContextData
		{
			VkInstance instance = VK_NULL_HANDLE;
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			unsigned int width = 0;
			unsigned int height = 0;

			bool recreatingSwapchain = false;

			uint32_t imageIndex = 0;

			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

			std::unique_ptr<VulkanDevice> device;
			std::unique_ptr<VulkanSwapchain> swapchain;
			std::unique_ptr<VulkanRenderPass> mainRenderPass;

			std::vector<std::unique_ptr<VulkanCommandBuffer>> graphicsCommandBuffer;

			struct FrameSyncObjects
			{
				std::unique_ptr<VulkanSemaphore> imageAvailable;
				std::unique_ptr<VulkanSemaphore> renderFinished;
				std::unique_ptr<VulkanFence> inFlightFence;
			};

			std::vector<FrameSyncObjects> frameSyncObjects;
			std::vector<VulkanFence*> imagesInFlight;

			bool enableValidation = true;
			const std::vector<const char*> validationLayers = {
				"VK_LAYER_KHRONOS_validation"
			};

			std::unique_ptr<VulkanAllocator> allocator;

			std::vector<std::unique_ptr<VulkanCommandBuffer>> frameCommandBuffers;
			std::vector<std::unique_ptr<VulkanFence>> frameFences;
		};

	public:
		static VulkanContextData Context;
	};
}
