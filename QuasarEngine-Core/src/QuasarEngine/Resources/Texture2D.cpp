#include "qepch.h"
#include "Texture2D.h"

#include <fstream>

#include <QuasarEngine/Renderer/Renderer.h>

#include <Platform/Vulkan/VulkanTexture2D.h>
#include <Platform/OpenGL/OpenGLTexture2D.h>
#include <Platform/DirectX/DirectXTexture2D.h>

namespace QuasarEngine
{
	std::shared_ptr<Texture2D> Texture2D::CreateTexture2D(const TextureSpecification& specification)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::Vulkan: return std::make_shared<VulkanTexture2D>(specification);
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTexture2D>(specification);
		case RendererAPI::API::DirectX: return std::make_shared<DirectXTexture2D>(specification);
		}

		return nullptr;
	}

	unsigned char* readFile(const std::string& filename, size_t* file_size) {
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			return nullptr;
		}

		file.seekg(0, std::ios::end);
		std::streampos size = file.tellg();
		file.seekg(0, std::ios::beg);

		*file_size = size;

		unsigned char* data = new unsigned char[size];

		file.read((char*)data, size);
		file.close();
		return data;
	}

	Texture2D::Texture2D(const TextureSpecification& spec) : Texture(spec)
	{

	}

	unsigned char* Texture2D::LoadDataFromPath(const std::string& path, size_t* file_size)
	{
		return readFile(path, file_size);
	}
}