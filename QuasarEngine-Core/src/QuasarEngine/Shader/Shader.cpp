#include "qepch.h"

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <Platform/Vulkan/VulkanShader.h>
#include <Platform/OpenGL/OpenGLShader.h>

namespace QuasarEngine
{
	std::shared_ptr<Shader> Shader::Create(const ShaderDescription& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan:	return std::make_shared<VulkanShader>(desc);
		case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLShader>(desc);
		}

		return nullptr;
	}
}