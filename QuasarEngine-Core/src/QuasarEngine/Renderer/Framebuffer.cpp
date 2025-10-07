#include "qepch.h"
#include "Framebuffer.h"

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <Platform/Vulkan/VulkanFramebuffer.h>
#include <Platform/OpenGL/OpenGLFramebuffer.h>
#include <Platform/DirectX/DirectXFramebuffer.h>

namespace QuasarEngine
{
	std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanFramebuffer>(spec);
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
		case RendererAPI::API::DirectX:  return std::make_shared<DirectXFramebuffer>(spec);
		}

		return nullptr;
	}

	Framebuffer::Framebuffer(const FramebufferSpecification& specification) : m_Specification(specification)
	{
		
	}
}