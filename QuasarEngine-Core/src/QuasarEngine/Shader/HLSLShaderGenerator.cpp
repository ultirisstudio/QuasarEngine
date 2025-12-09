#include "qepch.h"

#include <sstream>

#include <QuasarEngine/Shader/HLSLShaderGenerator.h>

namespace QuasarEngine
{
    void HLSLShaderGenerator::GenerateModules(Shader::ShaderDescription& desc)
    {
        for (auto& module : desc.modules)
        {
            if (!module.path.empty())
                continue;

            switch (module.stage)
            {
            case Shader::ShaderStageType::Vertex:
                module.source = GenerateVertex(desc, module);
                break;

            case Shader::ShaderStageType::Fragment:
                module.source = GenerateFragment(desc, module);
                break;

            case Shader::ShaderStageType::Compute:
                module.source = GenerateCompute(desc, module);
                break;

            default:
                break;
            }
        }
    }

    std::string HLSLShaderGenerator::GenerateVertex(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << "// HLSL vertex shader généré automatiquement (stub)\n";
        ss << "struct VS_INPUT\n";
        ss << "{\n";
        ss << "    float3 position : POSITION;\n";
        ss << "};\n\n";

        ss << "struct VS_OUTPUT\n";
        ss << "{\n";
        ss << "    float4 position : SV_POSITION;\n";
        ss << "};\n\n";

        ss << "VS_OUTPUT main(VS_INPUT input)\n";
        ss << "{\n";
        ss << "    VS_OUTPUT output;\n";
        ss << "    output.position = float4(input.position, 1.0f);\n";
        ss << "    return output;\n";
        ss << "}\n";

        return ss.str();
    }

    std::string HLSLShaderGenerator::GenerateFragment(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << "// HLSL pixel shader généré automatiquement (stub)\n";
        ss << "float4 main() : SV_TARGET\n";
        ss << "{\n";
        ss << "    return float4(1.0f, 0.0f, 1.0f, 1.0f);\n";
        ss << "}\n";

        return ss.str();
    }

    std::string HLSLShaderGenerator::GenerateCompute(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << "// HLSL compute shader généré automatiquement (stub)\n";
        ss << "[numthreads(8, 8, 1)]\n";
        ss << "void main(uint3 DTid : SV_DispatchThreadID)\n";
        ss << "{\n";
        ss << "    // TODO: compute shader généré\n";
        ss << "}\n";

        return ss.str();
    }
}
