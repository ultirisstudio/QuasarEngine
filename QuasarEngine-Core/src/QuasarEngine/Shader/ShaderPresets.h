#pragma once

#include <QuasarEngine/Shader/ShaderConfig.h>

namespace QuasarEngine::ShaderPresets
{
    using ::QuasarEngine::ShaderConfig;
    using ::QuasarEngine::ShaderPreset;
    using ::QuasarEngine::VertexLayout;
    using ::QuasarEngine::ShaderRenderPass;

    inline ShaderConfig PBRStatic()
    {
        ShaderConfig cfg;
        cfg.preset = ShaderPreset::PBR_Static;
        cfg.vertexLayout = VertexLayout::PBR_Static;
        cfg.renderPass = ShaderRenderPass::Default;

        cfg.features.usePBR = true;
        cfg.features.useIBL = true;
        cfg.features.useLights = true;
        cfg.features.useShadows = false;
        cfg.features.useSkinning = false;
        cfg.features.useTessellation = false;

        cfg.cullMode = Shader::CullMode::Back;
        cfg.fillMode = Shader::FillMode::Solid;
        cfg.blendMode = Shader::BlendMode::None;
        cfg.depthFunc = Shader::DepthFunc::Less;
        cfg.depthTestEnable = true;
        cfg.depthWriteEnable = true;
        cfg.topology = Shader::PrimitiveTopology::TriangleList;
        cfg.enableDynamicViewport = true;
        cfg.enableDynamicScissor = true;
        cfg.enableDynamicLineWidth = false;

        cfg.shaderName = "basic";

        return cfg;
    }

    inline ShaderConfig PBRSkin()
    {
        ShaderConfig cfg = PBRStatic();
        cfg.preset = ShaderPreset::PBR_Skinned;
        cfg.vertexLayout = VertexLayout::PBR_Skinned;

        cfg.features.useSkinning = true;

        cfg.shaderName = "basic_anim";

        return cfg;
    }

    inline ShaderConfig TerrainTessellated()
    {
        ShaderConfig cfg;
        cfg.preset = ShaderPreset::Terrain_Tessellated;
        cfg.vertexLayout = VertexLayout::Terrain;
        cfg.renderPass = ShaderRenderPass::Default;

        cfg.features.usePBR = true;
        cfg.features.useIBL = true;
        cfg.features.useLights = true;
        cfg.features.useTessellation = true;
        cfg.features.useTerrainDisplacement = true;

        cfg.cullMode = Shader::CullMode::Back;
        cfg.fillMode = Shader::FillMode::Solid;
        cfg.blendMode = Shader::BlendMode::None;
        cfg.depthFunc = Shader::DepthFunc::Less;
        cfg.depthTestEnable = true;
        cfg.depthWriteEnable = true;
        cfg.topology = Shader::PrimitiveTopology::PatchList;
        cfg.patchControlPoints = 4;

        cfg.shaderName = "gpuheight";

        return cfg;
    }

    inline ShaderConfig SkyboxPBR()
    {
        ShaderConfig cfg;
        cfg.preset = ShaderPreset::Skybox_PBR;
        cfg.vertexLayout = VertexLayout::Skybox;
        cfg.renderPass = ShaderRenderPass::Default;

        cfg.features.usePBR = false;
        cfg.features.useIBL = true;
        cfg.features.useLights = false;
        cfg.features.useSkyboxInput = true;

        cfg.cullMode = Shader::CullMode::Front;
        cfg.fillMode = Shader::FillMode::Solid;
        cfg.blendMode = Shader::BlendMode::None;
        cfg.depthFunc = Shader::DepthFunc::LessOrEqual;
        cfg.depthTestEnable = true;
        cfg.depthWriteEnable = false;

        cfg.topology = Shader::PrimitiveTopology::TriangleList;
        cfg.shaderName = "background";

        return cfg;
    }

    inline ShaderConfig FullscreenQuad(const std::string& shaderName)
    {
        ShaderConfig cfg;
        cfg.preset = ShaderPreset::Fullscreen_Quad;
        cfg.vertexLayout = VertexLayout::Fullscreen_Quad;
        cfg.renderPass = ShaderRenderPass::PostProcess;

        cfg.features.usePBR = false;
        cfg.features.useIBL = false;
        cfg.features.useLights = false;

        cfg.cullMode = Shader::CullMode::None;
        cfg.fillMode = Shader::FillMode::Solid;
        cfg.blendMode = Shader::BlendMode::None;
        cfg.depthTestEnable = false;
        cfg.depthWriteEnable = false;

        cfg.topology = Shader::PrimitiveTopology::TriangleList;
        cfg.isFullscreenPass = true;

        cfg.shaderName = shaderName;

        return cfg;
    }
}
