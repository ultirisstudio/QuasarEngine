#pragma once

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Shader/ShaderConfig.h>

namespace QuasarEngine
{
    class ShaderBuilder
    {
    public:
        ShaderBuilder() = default;

        ShaderBuilder& SetConfig(const ShaderConfig& cfg)
        {
            m_Config = cfg;
            return *this;
        }

        const ShaderConfig& GetConfig() const { return m_Config; }

        Shader::ShaderDescription Build() const;

        static Shader::ShaderDescription BuildDescription(const ShaderConfig& cfg)
        {
            ShaderBuilder b;
            b.SetConfig(cfg);
            return b.Build();
        }

    private:
        ShaderConfig m_Config{};

        void BuildPipelineState(Shader::ShaderDescription& desc) const;
        void BuildModules(Shader::ShaderDescription& desc) const;

        void BuildUniforms(Shader::ShaderDescription& desc) const;
        void BuildSamplers(Shader::ShaderDescription& desc) const;
        void BuildPushConstants(Shader::ShaderDescription& desc) const;
        void BuildStorageBuffers(Shader::ShaderDescription& desc) const;
    };
}
