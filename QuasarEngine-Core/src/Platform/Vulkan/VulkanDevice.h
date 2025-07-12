#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine {

	class VulkanDevice
	{
	public:
		VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
		~VulkanDevice();

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		struct QueueFamilyIndices {
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool IsComplete() const {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		QueueFamilyIndices GetQueueFamilyIndices() const { return queueIndices; }

		void PickPhysicalDevice();
		bool IsDeviceSuitable(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		void CreateLogicalDevice();
		void CreateGraphicsCommandPool();

		VkInstance instance;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;

		VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;

		QueueFamilyIndices queueIndices;

		bool supportsDeviceLocalHostVisible;

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		struct PhysicalDeviceInfo {
			std::string deviceName;
			uint32_t apiVersion;
			uint32_t driverVersion;
			uint32_t vendorID;
			uint32_t deviceID;
			VkPhysicalDeviceType deviceType;

			VkPhysicalDeviceLimits limits;
			VkPhysicalDeviceSparseProperties sparseProperties;

			uint32_t memoryTypeCount;
			std::vector<VkMemoryType> memoryTypes;

			uint32_t memoryHeapCount;
			std::vector<VkMemoryHeap> memoryHeaps;

			VkPhysicalDeviceFeatures features;

			static PhysicalDeviceInfo FromPhysicalDevice(VkPhysicalDevice device) {
				PhysicalDeviceInfo info{};

				VkPhysicalDeviceProperties props{};
				vkGetPhysicalDeviceProperties(device, &props);

				info.deviceName = props.deviceName;
				info.apiVersion = props.apiVersion;
				info.driverVersion = props.driverVersion;
				info.vendorID = props.vendorID;
				info.deviceID = props.deviceID;
				info.deviceType = props.deviceType;
				info.limits = props.limits;
				info.sparseProperties = props.sparseProperties;

				VkPhysicalDeviceMemoryProperties memProps{};
				vkGetPhysicalDeviceMemoryProperties(device, &memProps);

				info.memoryTypeCount = memProps.memoryTypeCount;
				info.memoryTypes.resize(memProps.memoryTypeCount);
				for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
					info.memoryTypes[i] = memProps.memoryTypes[i];
				}

				info.memoryHeapCount = memProps.memoryHeapCount;
				info.memoryHeaps.resize(memProps.memoryHeapCount);
				for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
					info.memoryHeaps[i] = memProps.memoryHeaps[i];
				}

				vkGetPhysicalDeviceFeatures(device, &info.features);

				return info;
			}

			std::string ToString() const {
				std::ostringstream oss;

				oss << "\n=== Physical Device Info ===\n";
				oss << "Device Name: " << deviceName << "\n";
				oss << "API Version: " << VK_VERSION_MAJOR(apiVersion) << "."
					<< VK_VERSION_MINOR(apiVersion) << "."
					<< VK_VERSION_PATCH(apiVersion) << "\n";
				oss << "Driver Version: " << std::hex << "0x" << driverVersion << std::dec << "\n";
				oss << "Vendor ID: " << vendorID << " | Device ID: " << deviceID << "\n";

				std::string typeStr;
				switch (deviceType) {
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: typeStr = "Integrated GPU"; break;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: typeStr = "Discrete GPU"; break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: typeStr = "Virtual GPU"; break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: typeStr = "CPU"; break;
				default: typeStr = "Other"; break;
				}
				oss << "Device Type: " << typeStr << "\n";

				oss << "\n--- Limits ---\n";
				oss << "Max Image Dimension 2D: " << limits.maxImageDimension2D << "\n";
				oss << "Max Uniform Buffer Range: " << limits.maxUniformBufferRange << "\n";
				oss << "Min Uniform Buffer Offset Alignment: " << limits.minUniformBufferOffsetAlignment << "\n";
				oss << "Max Push Constants Size: " << limits.maxPushConstantsSize << "\n";

				oss << "\n--- Memory ---\n";
				oss << "Memory Types (" << memoryTypeCount << "):\n";
				for (uint32_t i = 0; i < memoryTypeCount; ++i) {
					oss << "  [" << i << "] HeapIndex=" << memoryTypes[i].heapIndex
						<< " | PropertyFlags=" << std::hex << "0x" << memoryTypes[i].propertyFlags << std::dec << "\n";
				}

				oss << "Memory Heaps (" << memoryHeapCount << "):\n";
				for (uint32_t i = 0; i < memoryHeapCount; ++i) {
					oss << "  [" << i << "] Size=" << (memoryHeaps[i].size / (1024 * 1024)) << " MB"
						<< " | Flags=" << std::hex << "0x" << memoryHeaps[i].flags << std::dec << "\n";
				}

				oss << "\n--- Features ---\n";
				oss << "Geometry Shader: " << (features.geometryShader ? "Yes" : "No") << "\n";
				oss << "Tessellation Shader: " << (features.tessellationShader ? "Yes" : "No") << "\n";
				oss << "Multi Draw Indirect: " << (features.multiDrawIndirect ? "Yes" : "No") << "\n";
				oss << "Sampler Anisotropy: " << (features.samplerAnisotropy ? "Yes" : "No") << "\n";

				return oss.str();
			}

			friend std::ostream& operator<<(std::ostream& os, const PhysicalDeviceInfo& info) {
				return os << info.ToString();
			}
		};

		PhysicalDeviceInfo physicalDeviceInfo;
	};
}