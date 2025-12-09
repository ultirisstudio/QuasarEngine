#include "qepch.h"

#include <QuasarEngine/Shader/VulkanShaderGenerator.h>

namespace QuasarEngine
{
    std::string VulkanShaderGenerator::GetVersionDirective(Shader::ShaderStageType) const
    {
        return "#version 450\n";
    }
}
