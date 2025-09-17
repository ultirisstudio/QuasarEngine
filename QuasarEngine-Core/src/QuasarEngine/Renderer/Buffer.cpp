#include "qepch.h"
#include "QuasarEngine/Renderer/Buffer.h"
#include "QuasarEngine/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/DirectX/DirectXBuffer.h"

namespace QuasarEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>();
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>();
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXVertexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(size);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(size);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXVertexBuffer>(size);
		}

		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(data, size);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(data, size);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXVertexBuffer>(data, size);
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
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXIndexBuffer>();
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(size);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(size);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXIndexBuffer>(size);
		}

		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const void* data, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(data, size);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(data, size);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXIndexBuffer>(data, size);
		}

		return nullptr;
	}

}