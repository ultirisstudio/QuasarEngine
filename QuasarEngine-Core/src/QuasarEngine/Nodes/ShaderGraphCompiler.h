#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Nodes/NodeGraph.h>

namespace QuasarEngine
{
    enum class UniformDomain
    {
        Global,
        Object,
        PushConstant
    };

    struct UsedUniform
    {
        std::string name;
        Shader::ShaderUniformType type;
        UniformDomain domain;
    };

    struct UsedSampler
    {
        std::string name;
        Shader::SamplerType samplerType;
        Shader::ShaderStageFlags stages;
    };

    struct ShaderGraphResult
    {
        std::string vertexSource;
        std::string fragmentSource;

        std::vector<UsedUniform> uniforms;
        std::vector<UsedSampler> samplers;

        Node::NodeId materialOutputId = 0;
    };

    struct ShaderGraphCompileOptions
    {
        std::string shaderName = "generated_material";

        bool usePBRTemplate = true;
    };

    class ShaderGraphCompiler
    {
    public:
        static ShaderGraphResult CompileMaterialPBR(
            const NodeGraph& graph,
            Node::NodeId materialOutputId,
            const ShaderGraphCompileOptions& options);
    };
}