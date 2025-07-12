#include "qepch.h"
#include "QuasarEngine/Renderer/GraphicsContext.h"

#include "QuasarEngine/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace QuasarEngine {

	std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		return nullptr;
	}

}