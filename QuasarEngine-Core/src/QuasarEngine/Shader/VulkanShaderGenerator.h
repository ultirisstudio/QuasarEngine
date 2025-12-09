#pragma once

#include <QuasarEngine/Shader/GLSLShaderGenerator.h>

namespace QuasarEngine
{
    class VulkanShaderGenerator : public GLSLShaderGenerator
    {
    protected:
        std::string GetVersionDirective(Shader::ShaderStageType stage) const override;
    };
}
