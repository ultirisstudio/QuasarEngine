#pragma once

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
    class IShaderGenerator
    {
    public:
        virtual ~IShaderGenerator() = default;

        virtual void GenerateModules(Shader::ShaderDescription& desc) = 0;
    };
}
