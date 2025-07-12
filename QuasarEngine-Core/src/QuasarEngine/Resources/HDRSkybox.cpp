#include "qepch.h"

#include "HDRSkybox.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <Platform/Vulkan/VulkanHDRSkybox.h>

namespace QuasarEngine
{
	std::shared_ptr<HDRSkybox> HDRSkybox::CreateHDRSkybox()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:	return std::make_shared<VulkanHDRSkybox>();
		}

		return nullptr;
	}
}