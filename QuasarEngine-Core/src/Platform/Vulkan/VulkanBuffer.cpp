#include "qepch.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanShader.h" // TODO: Remove

namespace QuasarEngine
{

	VulkanBuffer::VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint64_t size, VkBufferUsageFlags usage, uint32_t memoryPropertyFlags, bool bindOnCreate)
		: handle(VK_NULL_HANDLE), device(device), physicalDevice(physicalDevice), memory(VK_NULL_HANDLE), totalSize(size), usage(usage), memoryPropertyFlags(memoryPropertyFlags)
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(device, &buffer_info, VulkanContext::Context.allocator->GetCallbacks(), &handle));

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(device, handle, &requirements);

		memoryIndex = FindMemoryIndex(requirements.memoryTypeBits, memoryPropertyFlags);
		if (memoryIndex == -1)
		{
			Q_ERROR("Unable to create vulkan buffer because the required memory type index was not found.");
			return;
		}

		if (!CanAllocate(requirements.size))
		{
			return;
		}

		VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocate_info.allocationSize = requirements.size;
		allocate_info.memoryTypeIndex = static_cast<uint32_t>(memoryIndex);

		VkResult result = vkAllocateMemory(device, &allocate_info, VulkanContext::Context.allocator->GetCallbacks(), &memory);

		if (result != VK_SUCCESS)
		{
			Q_ERROR("Unable to create vulkan buffer because the required memory type index was not found. Error: " + result);
			return;
		}

		if (bindOnCreate)
		{
			Bind();
		}

		std::string str = "Vulkan buffer initialized successfully with size: " + std::to_string(size);
		Q_DEBUG(str);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		vkDeviceWaitIdle(device);

		if (memory)
		{
			vkFreeMemory(device, memory, VulkanContext::Context.allocator->GetCallbacks());
			memory = VK_NULL_HANDLE;
		}

		if (handle)
		{
			vkDestroyBuffer(device, handle, VulkanContext::Context.allocator->GetCallbacks());
			handle = VK_NULL_HANDLE;
		}

		totalSize = 0;
		usage = 0;
		isLocked = false;
	}

	int32_t VulkanBuffer::FindMemoryIndex(uint32_t typeFilter, uint32_t propertyFlags)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memory_properties);

		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
			{
				return i;
			}
		}

		Q_WARNING("Unable to find suitable memory type !");
		return -1;
	}

	void VulkanBuffer::Resize(uint64_t newSize, VkQueue queue, VkCommandPool pool)
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = newSize;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer new_buffer;
		VK_CHECK(vkCreateBuffer(device, &buffer_info, VulkanContext::Context.allocator->GetCallbacks(), &new_buffer));

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(device, new_buffer, &requirements);

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		uint32_t heapIndex = memoryProperties.memoryTypes[memoryIndex].heapIndex;
		VkDeviceSize heapSize = memoryProperties.memoryHeaps[heapIndex].size;

		if (!CanAllocate(requirements.size))
		{
			vkDestroyBuffer(device, new_buffer, VulkanContext::Context.allocator->GetCallbacks());
			return;
		}

		VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocate_info.allocationSize = requirements.size;
		allocate_info.memoryTypeIndex = static_cast<uint32_t>(memoryIndex);

		VkDeviceMemory new_memory;
		VkResult result = vkAllocateMemory(device, &allocate_info, VulkanContext::Context.allocator->GetCallbacks(), &new_memory);

		if (result != VK_SUCCESS)
		{
			Q_ERROR("Unable to create vulkan buffer because the required memory type index was not found. Error: " + result);
			return;
		}

		VK_CHECK(vkBindBufferMemory(device, new_buffer, new_memory, 0));

		CopyTo(pool, 0, queue, handle, 0, new_buffer, 0, totalSize);

		vkDeviceWaitIdle(device);

		if (memory)
		{
			vkFreeMemory(device, memory, VulkanContext::Context.allocator->GetCallbacks());
			memory = VK_NULL_HANDLE;
		}

		if (handle)
		{
			vkDestroyBuffer(device, handle, VulkanContext::Context.allocator->GetCallbacks());
			handle = VK_NULL_HANDLE;
		}

		totalSize = newSize;
		memory = new_memory;
		handle = new_buffer;
	}

	bool VulkanBuffer::CanAllocate(VkDeviceSize size) const {
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		uint32_t heapIndex = memoryProperties.memoryTypes[memoryIndex].heapIndex;
		VkDeviceSize heapSize = memoryProperties.memoryHeaps[heapIndex].size;

		if (size > heapSize)
		{
			Q_ERROR("Unable to allocate Vulkan buffer: requested size (" + std::to_string(size) + ") exceeds heap size (" + std::to_string(heapSize) + ").");
			return false;
		}
		else
		{
			return true;
		}
	}

	void VulkanBuffer::Bind(uint64_t offset)
	{
		VK_CHECK(vkBindBufferMemory(device, handle, memory, offset));
	}

	void* VulkanBuffer::Lock(uint64_t offset, uint64_t size, uint32_t flags)
	{
		void* data;
		VK_CHECK(vkMapMemory(device, memory, offset, size, flags, &data));
		return data;
	}

	void VulkanBuffer::Unlock()
	{
		vkUnmapMemory(device, memory);
	}

	void VulkanBuffer::LoadData(uint64_t offset, uint64_t size, uint32_t flags, const void* data)
	{
		if (!data || size == 0)
		{
			Q_WARNING("Attempted to load null or zero-sized data into Vulkan buffer.");
			return;
		}

		if (offset + size > totalSize)
		{
			Q_ERROR("LoadData overflow: requested write from offset " + std::to_string(offset) + " with size " + std::to_string(size) + " exceeds total buffer size of " + std::to_string(totalSize));
			return;
		}

		//size_t alignment = VulkanContext::Context.device->alignment;

		//offset = (offset + alignment - 1) & ~(alignment - 1);

		void* data_ptr = nullptr;
		VK_CHECK(vkMapMemory(device, memory, offset, size, flags, &data_ptr));
		memcpy(data_ptr, data, size);
		vkUnmapMemory(device, memory);
	}

	void VulkanBuffer::CopyTo(VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer src, uint64_t srcOffset, VkBuffer dest, uint64_t destOffset, uint64_t size)
	{
		vkQueueWaitIdle(queue);

		std::unique_ptr<VulkanCommandBuffer> tempCommandBuffer = std::make_unique<VulkanCommandBuffer>(device, pool);
		tempCommandBuffer->AllocateAndBeginSingleUse();

		VkBufferCopy copy_buffer;
		copy_buffer.srcOffset = srcOffset;
		copy_buffer.dstOffset = destOffset;
		copy_buffer.size = size;

		vkCmdCopyBuffer(tempCommandBuffer->handle, src, dest, 1, &copy_buffer);

		tempCommandBuffer->EndSingleUse(queue);

		tempCommandBuffer.reset();
	}

	void VulkanBuffer::CopyTo(VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer dest, uint64_t destOffset, uint64_t size)
	{
		vkQueueWaitIdle(queue);

		std::unique_ptr<VulkanCommandBuffer> tempCommandBuffer = std::make_unique<VulkanCommandBuffer>(device, pool);
		tempCommandBuffer->AllocateAndBeginSingleUse();

		VkBufferCopy copy_buffer;
		copy_buffer.srcOffset = 0;
		copy_buffer.dstOffset = destOffset;
		copy_buffer.size = size;

		vkCmdCopyBuffer(tempCommandBuffer->handle, handle, dest, 1, &copy_buffer);

		tempCommandBuffer->EndSingleUse(queue);

		tempCommandBuffer.reset();
	}

	VulkanVertexBuffer::VulkanVertexBuffer()
		: currentOffset(0), m_Size(0)
	{
		uint64_t bufferSize = sizeof(glm::vec3) * (1024 * 1024);
		
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		std::string str = "VulkanVertexBuffer initialized with size: " + std::to_string(bufferSize);
		Q_DEBUG(str);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const std::vector<float> vertices)
		: currentOffset(0), m_Size(vertices.size())
	{
		uint64_t bufferSize = sizeof(float) * m_Size;

		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		std::string str = "VulkanVertexBuffer initialized with size: " + std::to_string(bufferSize);
		Q_DEBUG(str);

		UploadVertices(vertices);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		buffer.reset();
	}

	void VulkanVertexBuffer::Bind() const
	{
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(VulkanContext::Context.frameCommandBuffers.back()->handle, 0, 1, &buffer->handle, (VkDeviceSize*)offsets);
	}

	void VulkanVertexBuffer::Unbind() const
	{

	}

	void VulkanVertexBuffer::UploadVertices(const std::vector<float> vertices)
	{
		m_Size = vertices.size();

		uint64_t bufferSize = sizeof(float) * m_Size;

		std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true
		);

		staging->LoadData(0, bufferSize, 0, vertices.data());
		staging->CopyTo(VulkanContext::Context.device->graphicsCommandPool, 0, VulkanContext::Context.device->graphicsQueue, buffer->handle, currentOffset, bufferSize);

		currentOffset += bufferSize;
	}

	VulkanIndexBuffer::VulkanIndexBuffer()
		: currentOffset(0), m_Count(0)
	{
		uint64_t bufferSize = sizeof(uint32_t) * (1024 * 1024);

		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		std::string str = "VulkanIndexBuffer initialized with size: " + std::to_string(bufferSize);
		Q_DEBUG(str);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const std::vector<uint32_t> indices)
		: currentOffset(0), m_Count(indices.size())
	{
		uint64_t bufferSize = sizeof(uint32_t) * m_Count;

		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		std::string str = "VulkanIndexBuffer initialized with size: " + std::to_string(bufferSize);
		Q_DEBUG(str);

		UploadIndices(indices);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		buffer.reset();
	}

	void VulkanIndexBuffer::Bind() const
	{
		vkCmdBindIndexBuffer(VulkanContext::Context.frameCommandBuffers.back()->handle, buffer->handle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::Unbind() const
	{

	}

	void VulkanIndexBuffer::UploadIndices(const std::vector<uint32_t> indices)
	{
		m_Count = indices.size();

		uint64_t bufferSize = sizeof(uint32_t) * m_Count;

		std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true
		);

		staging->LoadData(0, bufferSize, 0, indices.data());
		staging->CopyTo(VulkanContext::Context.device->graphicsCommandPool, 0, VulkanContext::Context.device->graphicsQueue, buffer->handle, currentOffset, bufferSize);

		currentOffset += bufferSize;
	}
}