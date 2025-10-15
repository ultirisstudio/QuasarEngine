#include "qepch.h"
#include "TextureCubeMap.h"

#include <QuasarEngine/Renderer/RendererAPI.h>

#include <Platform/Vulkan/VulkanTextureCubeMap.h>
#include <Platform/OpenGL/OpenGLTextureCubeMap.h>
#include <Platform/DirectX/DirectXTextureCubeMap.h>

namespace QuasarEngine
{
    TextureCubeMap::TextureCubeMap(const TextureSpecification& specification) : Texture(specification) {}

    std::shared_ptr<TextureCubeMap> TextureCubeMap::Create(const TextureSpecification& specification) {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::None:   return nullptr;
        case RendererAPI::API::Vulkan: return std::make_shared<VulkanTextureCubeMap>(specification);
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTextureCubeMap>(specification);
        case RendererAPI::API::DirectX:return std::make_shared<DirectXTextureCubeMap>(specification);
        }
        return nullptr;
    }
}