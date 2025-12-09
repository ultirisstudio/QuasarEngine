#pragma once

#include <memory>

#include <QuasarEngine/Shader/IShaderGenerator.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

namespace QuasarEngine
{
    std::unique_ptr<IShaderGenerator> CreateShaderGenerator(RendererAPI::API api);
}
