#include "qepch.h"
#include "Framebuffer.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <Platform/Vulkan/VulkanFramebuffer.h>

namespace QuasarEngine
{
	std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanFramebuffer>(spec);
		}

		return nullptr;
	}
}