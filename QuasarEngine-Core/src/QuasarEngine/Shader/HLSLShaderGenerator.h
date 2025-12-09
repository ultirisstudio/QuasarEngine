#pragma once

#include <string>
#include <QuasarEngine/Shader/IShaderGenerator.h>

namespace QuasarEngine
{
    class HLSLShaderGenerator : public IShaderGenerator
    {
    public:
        void GenerateModules(Shader::ShaderDescription& desc) override;

    private:
        std::string GenerateVertex(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateFragment(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateCompute(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;
    };
}
