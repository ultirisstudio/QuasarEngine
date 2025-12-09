#pragma once

#include <string>

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
    enum class ShaderPreset
    {
        Custom = 0,

        PBR_Static,
        PBR_Skinned,

        Terrain_Tessellated,

        Skybox_PBR,

        Fullscreen_Quad,
        Debug_Normals,
        Debug_Depth
    };

    enum class VertexLayout
    {
        Unknown = 0,
        PBR_Static,
        PBR_Skinned,
        Terrain,
        Skybox,
        Fullscreen_Quad
    };

    enum class ShaderRenderPass
    {
        Default = 0,
        Shadow,
        DepthOnly,
        UI,
        PostProcess,
        Custom
    };

    struct ShaderFeatures
    {
        bool usePBR = false;
        bool useIBL = false;
        bool useLights = true;
        bool usePointLights = true;
        bool useDirectionalLights = true;

        bool useShadows = false;
        bool useFog = false;

        bool useSkinning = false;
        bool useVertexColors = false;
        bool useInstancing = false;

        bool useTessellation = false;
        bool useTerrainDisplacement = false;

        bool useAlphaClipping = false;
        bool useSkyboxInput = false;

        bool useEnvironmentBRDF = false;
    };

    struct ShaderConfig
    {
        ShaderPreset     preset = ShaderPreset::Custom;
        VertexLayout     vertexLayout = VertexLayout::Unknown;
        ShaderRenderPass renderPass = ShaderRenderPass::Default;

        ShaderFeatures   features;

        Shader::CullMode          cullMode = Shader::CullMode::Back;
        Shader::FillMode          fillMode = Shader::FillMode::Solid;
        Shader::BlendMode         blendMode = Shader::BlendMode::None;
        Shader::DepthFunc         depthFunc = Shader::DepthFunc::Less;
        bool                      depthTestEnable = true;
        bool                      depthWriteEnable = true;
        Shader::PrimitiveTopology topology = Shader::PrimitiveTopology::TriangleList;
        bool                      enableDynamicViewport = true;
        bool                      enableDynamicScissor = true;
        bool                      enableDynamicLineWidth = false;
        int                       patchControlPoints = 1;

        bool                      isFullscreenPass = false;
        bool                      isCompute = false;

        std::string shaderName;

        bool useSeparateShaderNames = false;
        std::string vertexShaderName;
        std::string tessControlShaderName;
        std::string tessEvalShaderName;
        std::string geometryShaderName;
        std::string fragmentShaderName;
        std::string computeShaderName;

        std::string vertexPathOverride;
        std::string tessControlPathOverride;
        std::string tessEvalPathOverride;
        std::string geometryPathOverride;
        std::string fragmentPathOverride;
        std::string computePathOverride;

        std::string basePathGL = "Assets/Shaders/gl/";
        std::string basePathVK = "Assets/Shaders/vk/spv/";
        std::string basePathDX = "Assets/Shaders/dx/";
    };
}
