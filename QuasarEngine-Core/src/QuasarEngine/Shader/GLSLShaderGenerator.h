#pragma once

#include <string>
#include <QuasarEngine/Shader/IShaderGenerator.h>

namespace QuasarEngine
{
    class GLSLShaderGenerator : public IShaderGenerator
    {
    public:
        void GenerateModules(Shader::ShaderDescription& desc) override;

    protected:
        virtual std::string GetVersionDirective(Shader::ShaderStageType stage) const;

        std::string GenerateVertex(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateFragment(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateTessControl(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateTessEval(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateGeometry(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;

        std::string GenerateCompute(const Shader::ShaderDescription& desc,
            const Shader::ShaderModuleInfo& module) const;
    };
}
