#include "qepch.h"
#include "TextureArray.h"

#include <QuasarEngine/Renderer/RendererAPI.h>

#include <Platform/Vulkan/VulkanTextureArray.h>
#include <Platform/OpenGL/OpenGLTextureArray.h>
#include <Platform/DirectX/DirectXTextureArray.h>

namespace QuasarEngine
{
    TextureArray::TextureArray(const TextureSpecification& specification) : Texture(specification) {}

    std::shared_ptr<TextureArray> TextureArray::Create(const TextureSpecification& specification) {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::None:   return nullptr;
        case RendererAPI::API::Vulkan: return std::make_shared<VulkanTextureArray>(specification);
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTextureArray>(specification);
        case RendererAPI::API::DirectX:return std::make_shared<DirectXTextureArray>(specification);
        }
        return nullptr;
    }
}