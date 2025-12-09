#include "qepch.h"

#include <QuasarEngine/Shader/ShaderGeneratorFactory.h>

#include <QuasarEngine/Shader/GLSLShaderGenerator.h>
#include <QuasarEngine/Shader/VulkanShaderGenerator.h>
#include <QuasarEngine/Shader/HLSLShaderGenerator.h>

namespace QuasarEngine
{
    std::unique_ptr<IShaderGenerator> CreateShaderGenerator(RendererAPI::API api)
    {
        switch (api)
        {
        case RendererAPI::API::OpenGL:
            return std::make_unique<GLSLShaderGenerator>();

        case RendererAPI::API::Vulkan:
            return std::make_unique<VulkanShaderGenerator>();

        case RendererAPI::API::DirectX:
            return std::make_unique<HLSLShaderGenerator>();

        default:
            return nullptr;
        }
    }
}
