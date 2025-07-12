#include "qepch.h"

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <Platform/Vulkan/VulkanShader.h>

namespace QuasarEngine
{
	std::shared_ptr<Shader> Shader::Create(const ShaderDescription& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:	return std::make_shared<VulkanShader>(desc);
		}

		return nullptr;
	}
}