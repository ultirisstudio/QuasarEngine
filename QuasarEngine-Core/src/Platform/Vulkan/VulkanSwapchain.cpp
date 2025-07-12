#include "qepch.h"
#include "VulkanSwapchain.h"

#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace QuasarEngine
{
	VulkanSwapchain::VulkanSwapchain(VkExtent2D windowExtent)
		: currentFrame(0)
	{
		CreateSwapchain(windowExtent);
	}

	VulkanSwapchain::~VulkanSwapchain() {
		vkDeviceWaitIdle(VulkanContext::Context.device->device);

		depthAttachment.reset();

		Q_DEBUG("Destroying Vulkan views...");
		for (auto view : views) {
			vkDestroyImageView(VulkanContext::Context.device->device, view, VulkanContext::Context.allocator->GetCallbacks());
		}

		depthAttachment.reset();

		Q_DEBUG("Destroying Vulkan swapchain...");
		if (swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(VulkanContext::Context.device->device, swapchain, VulkanContext::Context.allocator->GetCallbacks());
		}
	}

	bool VulkanSwapchain::CheckDepthFormat()
	{
		const uint64_t candidate_count = 3;
		VkFormat candidates[3] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

		uint8_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		for (uint64_t i = 0; i < candidate_count; i++)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(VulkanContext::Context.device->physicalDevice, candidates[i], &props);

			if ((props.linearTilingFeatures & flags) == flags)
			{
				depthFormat = candidates[i];
				return true;
			}
			else if ((props.optimalTilingFeatures & flags) == flags)
			{
				depthFormat = candidates[i];
				return true;
			}
		}

		return false;
	}

	void VulkanSwapchain::CreateSwapchain(VkExtent2D windowExtent)
	{
		extent = windowExtent;

		swapchainSupport = QuerySwapchainSupport(VulkanContext::Context.device->physicalDevice);

		VkExtent2D swapchainExtent = { extent.width, extent.height };
		maxFramesInFlight = 2;

		bool found = false;
		for (uint32_t i = 0; i < swapchainSupport.formats.size(); ++i)
		{
			VkSurfaceFormatKHR format = swapchainSupport.formats[i];
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				imageFormat = format;
				found = true;
				break;
			}
		}

		if (!found)
		{
			imageFormat = swapchainSupport.formats[0];
		}

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < swapchainSupport.presentModes.size(); ++i)
		{
			VkPresentModeKHR mode = swapchainSupport.presentModes[i];
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = mode;
				break;
			}
		}

		swapchainSupport = QuerySwapchainSupport(VulkanContext::Context.device->physicalDevice);

		if (swapchainSupport.capabilities.currentExtent.width != UINT32_MAX)
		{
			swapchainExtent = swapchainSupport.capabilities.currentExtent;
		}
		else
		{
			swapchainExtent.width = std::max(swapchainSupport.capabilities.minImageExtent.width, std::min(swapchainSupport.capabilities.maxImageExtent.width, swapchainExtent.width));
			swapchainExtent.height = std::max(swapchainSupport.capabilities.minImageExtent.height, std::min(swapchainSupport.capabilities.maxImageExtent.height, swapchainExtent.height));
		}

		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

		VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		createInfo.surface = VulkanContext::Context.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = imageFormat.format;
		createInfo.imageColorSpace = imageFormat.colorSpace;
		createInfo.imageExtent = swapchainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t graphicsFamily = VulkanContext::Context.device->GetQueueFamilyIndices().graphicsFamily.value();
		uint32_t presentFamily = VulkanContext::Context.device->GetQueueFamilyIndices().presentFamily.value();

		if (graphicsFamily != presentFamily)
		{
			uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = 0;
		}

		createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(VulkanContext::Context.device->device, &createInfo, VulkanContext::Context.allocator->GetCallbacks(), &swapchain));

		currentFrame = 0;

		images.clear();
		views.clear();

		VK_CHECK(vkGetSwapchainImagesKHR(VulkanContext::Context.device->device, swapchain, &imageCount, 0));
		images.resize(imageCount);
		views.resize(imageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(VulkanContext::Context.device->device, swapchain, &imageCount, images.data()));

		for (uint32_t i = 0; i < imageCount; ++i)
		{
			VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			createInfo.image = images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = imageFormat.format;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(VulkanContext::Context.device->device, &createInfo, VulkanContext::Context.allocator->GetCallbacks(), &views[i]))
		}

		depthAttachment.reset();

		if (!CheckDepthFormat())
		{
			depthFormat = VK_FORMAT_UNDEFINED;
			Q_FATAL("Depth format not supported");
		}

		depthAttachment = std::make_unique<VulkanImage>(VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_DEPTH_BIT);

		Q_DEBUG("Vulkan swapchain created successfully");
	}

	void VulkanSwapchain::RecreateSwapchain(VkExtent2D windowExtent)
	{
		for (auto view : views) {
			vkDestroyImageView(VulkanContext::Context.device->device, view, VulkanContext::Context.allocator->GetCallbacks());
		}
		if (swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(VulkanContext::Context.device->device, swapchain, VulkanContext::Context.allocator->GetCallbacks());
		}

		CreateSwapchain(windowExtent);
	}

	bool VulkanSwapchain::AcquireNextImage(uint64_t timeout, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t* outImageIndex)
	{
		VkResult result = vkAcquireNextImageKHR(VulkanContext::Context.device->device, swapchain, timeout, imageAvailableSemaphore, fence, outImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain(extent);
			return false;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			Q_FATAL("Failed to acquire next image");
			return false;
		}

		return true;
	}

	void VulkanSwapchain::PresentImage(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore)
	{
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = 0;

		VkResult result = vkQueuePresentKHR(VulkanContext::Context.device->presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(extent);
		}
		else if (result != VK_SUCCESS)
		{
			Q_FATAL("Failed to present image");
		}

		currentFrame = (currentFrame + 1) % maxFramesInFlight;
	}

	VulkanSwapchain::SwapChainSupportDetails VulkanSwapchain::QuerySwapchainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VulkanContext::Context.surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext::Context.surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext::Context.surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext::Context.surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext::Context.surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
}
