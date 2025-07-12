#include "qepch.h"
#include "QuasarEngine/Renderer/Buffer.h"
#include "QuasarEngine/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanBuffer.h"

namespace QuasarEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const std::vector<float> vertices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(vertices);
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const std::vector<uint32_t> indices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(indices);
		}

		return nullptr;
	}

}