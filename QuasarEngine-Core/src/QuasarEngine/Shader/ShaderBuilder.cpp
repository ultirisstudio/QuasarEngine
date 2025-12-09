#include "qepch.h"

#include <QuasarEngine/Shader/ShaderBuilder.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

namespace QuasarEngine
{
    static std::string ShaderExtensionFor(RendererAPI::API api, Shader::ShaderStageType stage)
    {
        const bool isVulkan = (api == RendererAPI::API::Vulkan);

        switch (stage)
        {
        case Shader::ShaderStageType::Vertex:
            return isVulkan ? ".vert.spv" : ".vert.glsl";
        case Shader::ShaderStageType::TessControl:
            return isVulkan ? ".tesc.spv" : ".tesc.glsl";
        case Shader::ShaderStageType::TessEval:
            return isVulkan ? ".tese.spv" : ".tese.glsl";
        case Shader::ShaderStageType::Geometry:
            return isVulkan ? ".geom.spv" : ".geom.glsl";
        case Shader::ShaderStageType::Fragment:
            return isVulkan ? ".frag.spv" : ".frag.glsl";
        case Shader::ShaderStageType::Compute:
            return isVulkan ? ".comp.spv" : ".comp.glsl";
        default:
            return std::string{};
        }
    }

    static std::string BasePathForAPI(const ShaderConfig& cfg, RendererAPI::API api)
    {
        switch (api)
        {
        case RendererAPI::API::OpenGL:
            return cfg.basePathGL;
        case RendererAPI::API::Vulkan:
            return cfg.basePathVK;
        case RendererAPI::API::DirectX:
            return cfg.basePathDX;
        default:
            return cfg.basePathGL;
        }
    }

    static std::string BuildStagePath(const ShaderConfig& cfg,
        RendererAPI::API api,
        Shader::ShaderStageType stage)
    {
        switch (stage)
        {
        case Shader::ShaderStageType::Vertex:
            if (!cfg.vertexPathOverride.empty())
                return cfg.vertexPathOverride;
            break;
        case Shader::ShaderStageType::TessControl:
            if (!cfg.tessControlPathOverride.empty())
                return cfg.tessControlPathOverride;
            break;
        case Shader::ShaderStageType::TessEval:
            if (!cfg.tessEvalPathOverride.empty())
                return cfg.tessEvalPathOverride;
            break;
        case Shader::ShaderStageType::Geometry:
            if (!cfg.geometryPathOverride.empty())
                return cfg.geometryPathOverride;
            break;
        case Shader::ShaderStageType::Fragment:
            if (!cfg.fragmentPathOverride.empty())
                return cfg.fragmentPathOverride;
            break;
        case Shader::ShaderStageType::Compute:
            if (!cfg.computePathOverride.empty())
                return cfg.computePathOverride;
            break;
        default:
            break;
        }

        std::string name;

        if (cfg.useSeparateShaderNames)
        {
            switch (stage)
            {
            case Shader::ShaderStageType::Vertex:
                name = !cfg.vertexShaderName.empty() ? cfg.vertexShaderName : cfg.shaderName;
                break;
            case Shader::ShaderStageType::TessControl:
                name = !cfg.tessControlShaderName.empty() ? cfg.tessControlShaderName : cfg.shaderName;
                break;
            case Shader::ShaderStageType::TessEval:
                name = !cfg.tessEvalShaderName.empty() ? cfg.tessEvalShaderName : cfg.shaderName;
                break;
            case Shader::ShaderStageType::Geometry:
                name = !cfg.geometryShaderName.empty() ? cfg.geometryShaderName : cfg.shaderName;
                break;
            case Shader::ShaderStageType::Fragment:
                name = !cfg.fragmentShaderName.empty() ? cfg.fragmentShaderName : cfg.shaderName;
                break;
            case Shader::ShaderStageType::Compute:
                name = !cfg.computeShaderName.empty() ? cfg.computeShaderName : cfg.shaderName;
                break;
            default:
                break;
            }
        }
        else
        {
            name = cfg.shaderName;
        }

        if (name.empty())
            return std::string{};

        const auto basePath = BasePathForAPI(cfg, api);
        const auto ext = ShaderExtensionFor(api, stage);

        return basePath + name + ext;
    }

    Shader::ShaderDescription ShaderBuilder::Build() const
    {
        Shader::ShaderDescription desc{};

        BuildPipelineState(desc);
        BuildModules(desc);

        BuildUniforms(desc);
        BuildSamplers(desc);
        BuildPushConstants(desc);
        BuildStorageBuffers(desc);

        return desc;
    }

    void ShaderBuilder::BuildPipelineState(Shader::ShaderDescription& desc) const
    {
        desc.depthWriteEnable = m_Config.depthWriteEnable;
        desc.depthTestEnable = m_Config.depthTestEnable;
        desc.depthFunc = m_Config.depthFunc;

        desc.cullMode = m_Config.cullMode;
        desc.fillMode = m_Config.fillMode;
        desc.blendMode = m_Config.blendMode;
        desc.topology = m_Config.topology;

        desc.enableDynamicViewport = m_Config.enableDynamicViewport;
        desc.enableDynamicScissor = m_Config.enableDynamicScissor;
        desc.enableDynamicLineWidth = m_Config.enableDynamicLineWidth;

        desc.patchControlPoints = m_Config.patchControlPoints;

        desc.framebuffer = nullptr;
    }

    void ShaderBuilder::BuildModules(Shader::ShaderDescription& desc) const
    {
        const auto api = RendererAPI::GetAPI();

        std::vector<Shader::ShaderModuleInfo> modules;

        if (m_Config.isCompute)
        {
            const auto computePath = BuildStagePath(m_Config, api, Shader::ShaderStageType::Compute);
            if (!computePath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::Compute;
                mod.path = computePath;
                modules.push_back(std::move(mod));
            }

            desc.modules = std::move(modules);
            return;
        }

        {
            const auto vertPath = BuildStagePath(m_Config, api, Shader::ShaderStageType::Vertex);
            if (!vertPath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::Vertex;
                mod.path = vertPath;
                modules.push_back(std::move(mod));
            }
        }

        if (m_Config.features.useTessellation)
        {
            const auto tescPath = BuildStagePath(m_Config, api, Shader::ShaderStageType::TessControl);
            if (!tescPath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::TessControl;
                mod.path = tescPath;
                modules.push_back(std::move(mod));
            }

            const auto tesePath = BuildStagePath(m_Config, api, Shader::ShaderStageType::TessEval);
            if (!tesePath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::TessEval;
                mod.path = tesePath;
                modules.push_back(std::move(mod));
            }
        }

        if (m_Config.useSeparateShaderNames ? !m_Config.geometryShaderName.empty() : false)
        {
            const auto geomPath = BuildStagePath(m_Config, api, Shader::ShaderStageType::Geometry);
            if (!geomPath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::Geometry;
                mod.path = geomPath;
                modules.push_back(std::move(mod));
            }
        }

        {
            const auto fragPath = BuildStagePath(m_Config, api, Shader::ShaderStageType::Fragment);
            if (!fragPath.empty())
            {
                Shader::ShaderModuleInfo mod{};
                mod.stage = Shader::ShaderStageType::Fragment;
                mod.path = fragPath;
                modules.push_back(std::move(mod));
            }
        }

        desc.modules = std::move(modules);
    }

    void ShaderBuilder::BuildUniforms(Shader::ShaderDescription& desc) const
    {
        (void)desc;
    }

    void ShaderBuilder::BuildSamplers(Shader::ShaderDescription& desc) const
    {
        (void)desc;
    }

    void ShaderBuilder::BuildPushConstants(Shader::ShaderDescription& desc) const
    {
        (void)desc;
    }

    void ShaderBuilder::BuildStorageBuffers(Shader::ShaderDescription& desc) const
    {
        (void)desc;
    }
}
