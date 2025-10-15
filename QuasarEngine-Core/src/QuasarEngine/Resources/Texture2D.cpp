#include "qepch.h"
#include "Texture2D.h"

#include <QuasarEngine/Renderer/RendererAPI.h>

#include <Platform/Vulkan/VulkanTexture2D.h>
#include <Platform/OpenGL/OpenGLTexture2D.h>
#include <Platform/DirectX/DirectXTexture2D.h>

namespace QuasarEngine
{
	Texture2D::Texture2D(const TextureSpecification& specification) : Texture(specification) {}

	std::shared_ptr<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan: return std::make_shared<VulkanTexture2D>(specification);
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTexture2D>(specification);
		case RendererAPI::API::DirectX: return std::make_shared<DirectXTexture2D>(specification);
		}
		
		return nullptr;
	}
}