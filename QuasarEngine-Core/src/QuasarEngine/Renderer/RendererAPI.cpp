#include "qepch.h"

#include "QuasarEngine/Renderer/RendererAPI.h"

#include "Platform/Vulkan/VulkanRendererAPI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/DirectX/DirectXRendererAPI.h"

namespace QuasarEngine {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

	std::unique_ptr<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_unique<VulkanRendererAPI>();
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLRendererAPI>();
		case RendererAPI::API::DirectX:  return std::make_unique<DirectXRendererAPI>();
		}

		return nullptr;
	}

}