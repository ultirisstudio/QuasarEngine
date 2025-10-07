#include "qepch.h"
#include "QuasarEngine/Renderer/GraphicsContext.h"

#include "QuasarEngine/Renderer/RendererAPI.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/DirectX/DirectXContext.h"

namespace QuasarEngine {

	std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
		case RendererAPI::API::DirectX:  return std::make_unique<DirectXContext>(static_cast<GLFWwindow*>(window));
		}

		return nullptr;
	}

}