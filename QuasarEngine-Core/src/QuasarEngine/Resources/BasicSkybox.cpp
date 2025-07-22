#include "qepch.h"

#include "BasicSkybox.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <Platform/Vulkan/VulkanBasicSkybox.h>
#include <Platform/OpenGL/OpenGLBasicSkybox.h>

namespace QuasarEngine
{
	std::shared_ptr<BasicSkybox> BasicSkybox::CreateBasicSkybox()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:	return std::make_shared<VulkanBasicSkybox>();
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLBasicSkybox>();
		}

		return nullptr;
	}
}