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

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const std::vector<float> vertices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(vertices);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(vertices);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXVertexBuffer>(vertices);
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

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const std::vector<uint32_t> indices)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(indices);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(indices);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXIndexBuffer>(indices);
		}

		return nullptr;
	}

}