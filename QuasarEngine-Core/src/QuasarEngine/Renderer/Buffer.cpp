#include "qepch.h"
#include "QuasarEngine/Renderer/Buffer.h"
#include "QuasarEngine/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace QuasarEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>();
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const std::vector<float> vertices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(vertices);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(vertices);
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>();
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const std::vector<uint32_t> indices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(indices);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(indices);
		}

		return nullptr;
	}

}