#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
	class VulkanImage;
	class VulkanFramebuffer;

	class VulkanSwapchain
	{
	public:
		VulkanSwapchain(VkExtent2D windowExtent);
		~VulkanSwapchain();

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

		void CreateSwapchain(VkExtent2D windowExtent);
		void RecreateSwapchain(VkExtent2D windowExtent);
		bool AcquireNextImage(uint64_t timeout, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t* outImageIndex);
		void PresentImage(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore);

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		std::vector<VkImageView> views;
		//std::vector<std::unique_ptr<VulkanFramebuffer>> framebuffers;
		std::vector<VkFramebuffer> framebuffers;

		std::unique_ptr<VulkanImage> depthAttachment;

		SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

		bool CheckDepthFormat();

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		SwapChainSupportDetails swapchainSupport;

		VkSurfaceFormatKHR imageFormat;

		VkFormat depthFormat;

		VkExtent2D extent;

		uint32_t currentFrame;

		uint8_t maxFramesInFlight;

		std::vector<VkImage> images;
	};
}