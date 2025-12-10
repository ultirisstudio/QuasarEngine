#include "qepch.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"

namespace QuasarEngine
{
	static inline uint32_t NextPow2(uint32_t v) {
		if (v <= 1024) return 1024;
		v--;
		v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
		v++;
		return v;
	}

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
			Q_ERROR("Unable to create vulkan buffer because the required memory type index was not found. Error: " + VulkanResultString(result));
			if (handle != VK_NULL_HANDLE) {
				vkDestroyBuffer(device, handle, VulkanContext::Context.allocator->GetCallbacks());
				handle = VK_NULL_HANDLE;
			}
			return;
		}

		if (bindOnCreate)
		{
			Bind();
		}
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
			Q_ERROR("Unable to create vulkan buffer because the required memory type index was not found. Error: " + VulkanResultString(result));
			vkDestroyBuffer(device, new_buffer, VulkanContext::Context.allocator->GetCallbacks());
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
		if (offset != 0) {
			VkMemoryRequirements req{};
			vkGetBufferMemoryRequirements(device, handle, &req);
			if (offset % req.alignment != 0) {
				Q_ERROR("Bind offset not aligned to memory requirements alignment.");
				return;
			}
		}

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

		void* data_ptr = nullptr;
		VK_CHECK(vkMapMemory(device, memory, offset, size, flags, &data_ptr));
		memcpy(data_ptr, data, size);
		vkUnmapMemory(device, memory);
	}

	void VulkanBuffer::CopyTo(VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer src, uint64_t srcOffset, VkBuffer dest, uint64_t destOffset, uint64_t size)
	{
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
		: m_Size(0), currentOffset(0)
	{
		const uint32_t initialBytes = 64 * 1024;
		m_Size = initialBytes;

		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t sizeBytes)
		: m_Size(std::max<uint32_t>(sizeBytes, 1024)), currentOffset(0)
	{
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const void* data, uint32_t sizeBytes)
		: m_Size(std::max<uint32_t>(sizeBytes, 1024)), currentOffset(0)
	{
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		Upload(data, sizeBytes);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		buffer.reset();
	}

	void VulkanVertexBuffer::Bind() const
	{
		VkDeviceSize offsets[1] = { 0 };
		VkBuffer bufs[1] = { buffer->handle };
		vkCmdBindVertexBuffers(
			VulkanContext::Context.frameCommandBuffers.back()->handle, 0, 1, bufs, offsets
		);
	}

	void VulkanVertexBuffer::Unbind() const
	{

	}

	void VulkanVertexBuffer::Reserve(uint32_t size) {
		if (size <= m_Size) return;

		m_Size = NextPow2(size);

		buffer.reset();
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		currentOffset = 0;
	}

	void VulkanVertexBuffer::Upload(const void* data, uint32_t size) {
		if (size == 0 || data == nullptr) return;

		if (size > m_Size) {
			Reserve(size);
		}

		std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true
		);

		staging->LoadData(0, size, 0, data);

		staging->CopyTo(
			VulkanContext::Context.device->graphicsCommandPool,
			0,
			VulkanContext::Context.device->graphicsQueue,
			buffer->handle,
			0,
			size
		);

		currentOffset = size;
	}

	VulkanIndexBuffer::VulkanIndexBuffer()
		: m_Size(0), currentOffset(0)
	{
		const uint32_t initialBytes = 32 * 1024;
		m_Size = initialBytes;

		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: m_Size(std::max<uint32_t>(size, 1024)), currentOffset(0)
	{
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const void* data, uint32_t size)
		: m_Size(std::max<uint32_t>(size, 1024)), currentOffset(0)
	{
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		Upload(data, size);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		buffer.reset();
	}

	void VulkanIndexBuffer::Bind() const
	{
		vkCmdBindIndexBuffer(
			VulkanContext::Context.frameCommandBuffers.back()->handle, buffer->handle, 0, VK_INDEX_TYPE_UINT32
		);
	}

	void VulkanIndexBuffer::Unbind() const
	{

	}

	void VulkanIndexBuffer::Reserve(uint32_t sizeBytes) {
		if (sizeBytes <= m_Size) return;

		m_Size = NextPow2(sizeBytes);

		buffer.reset();
		buffer = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			m_Size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);

		currentOffset = 0;
	}

	void VulkanIndexBuffer::Upload(const void* data, uint32_t size) {
		if (size == 0 || data == nullptr) return;

		if (size > m_Size) {
			Reserve(size);
		}

		std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true
		);

		staging->LoadData(0, size, 0, data);

		staging->CopyTo(
			VulkanContext::Context.device->graphicsCommandPool,
			0,
			VulkanContext::Context.device->graphicsQueue,
			buffer->handle,
			0,
			size
		);

		currentOffset = size;
	}
}