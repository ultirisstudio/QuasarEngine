#include "qepch.h"

#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include <set>
#include <ostream>

namespace QuasarEngine
{
	VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
		: instance(instance), surface(surface), supportsDeviceLocalHostVisible(false)
	{
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateGraphicsCommandPool();

		Q_DEBUG("Vulkan device created successfully");
	}

	VulkanDevice::~VulkanDevice() {
		Q_DEBUG("Destroying Vulkan command pool...");
		vkDestroyCommandPool(device, graphicsCommandPool, VulkanContext::Context.allocator->GetCallbacks());

		Q_DEBUG("Destroying Vulkan device...");
		if (device != VK_NULL_HANDLE) {
			vkDestroyDevice(device, VulkanContext::Context.allocator->GetCallbacks());
		}
	}

	void VulkanDevice::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("No Vulkan-compatible GPUs found!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		int bestScore = -1;
		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

		for (const auto& device : devices)
		{
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceProperties(device, &properties);
			vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
				if (((memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) &&
					((memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0))
				{
					supportsDeviceLocalHostVisible = true;
					break;
				}
			}

			int score = 0;

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			uint64_t totalMemory = 0;
			for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
				if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
					totalMemory += memoryProperties.memoryHeaps[i].size;
				}
			}
			score += static_cast<int>(totalMemory / (1024 * 1024 * 1024));

			if (!IsDeviceSuitable(device)) {
				continue;
			}

			std::ostringstream oss;
			oss << "Device: " << properties.deviceName
				<< " | Type: " << properties.deviceType
				<< " | Score: " << score
				<< " | API Version: "
				<< VK_VERSION_MAJOR(properties.apiVersion) << "."
				<< VK_VERSION_MINOR(properties.apiVersion) << "."
				<< VK_VERSION_PATCH(properties.apiVersion);

			Q_DEBUG(oss.str());

			if (score > bestScore) {
				bestScore = score;
				bestDevice = device;
			}
		}

		if (bestDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		physicalDevice = bestDevice;
		queueIndices = FindQueueFamilies(physicalDevice);

		physicalDeviceInfo = PhysicalDeviceInfo::FromPhysicalDevice(physicalDevice);

		Q_DEBUG(physicalDeviceInfo.ToString());
	}

	bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& ext : availableExtensions) {
			requiredExtensions.erase(ext.extensionName);
		}

		return indices.IsComplete() && requiredExtensions.empty();
	}

	VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.IsComplete()) break;
			++i;
		}

		return indices;
	}

	void VulkanDevice::CreateLogicalDevice() {
		std::set<uint32_t> uniqueQueueFamilies = {
			queueIndices.graphicsFamily.value(),
			queueIndices.presentFamily.value()
		};

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		float priority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures features{};
		features.tessellationShader = physicalDeviceInfo.features.tessellationShader;
		features.samplerAnisotropy = physicalDeviceInfo.features.samplerAnisotropy;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &features;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (vkCreateDevice(physicalDevice, &createInfo, VulkanContext::Context.allocator->GetCallbacks(), &device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(device, queueIndices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueIndices.presentFamily.value(), 0, &presentQueue);

		Q_DEBUG("Logical device created successfully");
	}

	void VulkanDevice::CreateGraphicsCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK(vkCreateCommandPool(device, &poolInfo, VulkanContext::Context.allocator->GetCallbacks(), &graphicsCommandPool));

		Q_DEBUG("Command pool created successfully");
	}

	uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}
}
