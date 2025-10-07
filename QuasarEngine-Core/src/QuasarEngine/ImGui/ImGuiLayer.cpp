#include "qepch.h"
#include "ImGuiLayer.h"

#include "QuasarEngine/Renderer/RendererAPI.h"

#include "Platform/Vulkan/VulkanImGuiLayer.h"
#include "Platform/OpenGL/OpenGLImGuiLayer.h"
#include "Platform/DirectX/DirectXImGuiLayer.h"

namespace QuasarEngine
{
	ImGuiLayer::ImGuiLayer(const std::string& name) : Layer(name)
	{

	}

	std::unique_ptr<ImGuiLayer> ImGuiLayer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_unique<VulkanImGuiLayer>();
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLImGuiLayer>();
		case RendererAPI::API::DirectX:  return std::make_unique<DirectXImGuiLayer>();
		}

		return nullptr;
	}
}