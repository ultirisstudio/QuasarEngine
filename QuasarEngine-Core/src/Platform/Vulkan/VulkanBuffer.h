#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Renderer/Buffer.h>

#include <glm/glm.hpp>

namespace QuasarEngine
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint64_t size, VkBufferUsageFlags usage, uint32_t memoryPropertyFlags, bool bindOnCreate);
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		void Resize(uint64_t newSize, VkQueue queue, VkCommandPool pool);

		void Bind(uint64_t offset = 0);

		void* Lock(uint64_t offset, uint64_t size, uint32_t flags);
		void Unlock();

		void LoadData(uint64_t offset, uint64_t size, uint32_t flags, const void* data);

		void CopyTo(VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer src, uint64_t srcOffset, VkBuffer dest, uint64_t destOffset, uint64_t size);
		void CopyTo(VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer dest, uint64_t destOffset, uint64_t size);

		bool CanAllocate(VkDeviceSize size) const;

		VkBuffer handle;

		VkDevice device;
		VkPhysicalDevice physicalDevice;

		VkDeviceMemory memory;

		VkBufferUsageFlags usage;

		bool isLocked;

		int memoryIndex;
		uint64_t totalSize;
		uint32_t memoryPropertyFlags;

	private:
		int32_t FindMemoryIndex(uint32_t typeFilter, uint32_t propertyFlags); // TODO: centralize
		
	};

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer();
		VulkanVertexBuffer(const std::vector<float> vertices);
		~VulkanVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void UploadVertices(const std::vector<float> vertices) override;

		size_t GetSize() const override { return m_Size; }

		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
		std::unique_ptr<VulkanBuffer> buffer;

		size_t m_Size;
		uint64_t currentOffset;
		BufferLayout m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer();
		VulkanIndexBuffer(const std::vector<uint32_t> indices);
		~VulkanIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void UploadIndices(const std::vector<uint32_t> indices) override;

		size_t GetCount() const override { return m_Count; }
	private:
		std::unique_ptr<VulkanBuffer> buffer;

		size_t m_Count;
		uint64_t currentOffset;
	};
}