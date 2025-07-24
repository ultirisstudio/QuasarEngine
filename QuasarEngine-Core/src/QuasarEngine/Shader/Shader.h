#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "QuasarEngine/Renderer/Framebuffer.h"
#include "QuasarEngine/Resources/Texture2D.h"

namespace QuasarEngine
{
	class Material;

	class Shader
	{
	public:
        enum class ShaderStageType {
            Vertex,
            Fragment,
            Geometry,
            Compute,
            TessControl,
            TessEval
        };

        using ShaderStageFlags = uint32_t;
        static constexpr ShaderStageFlags StageToBit(ShaderStageType type) {
            switch (type) {
            case ShaderStageType::Vertex:      return 0x01;
            case ShaderStageType::Fragment:    return 0x02;
            case ShaderStageType::Geometry:    return 0x04;
            case ShaderStageType::Compute:     return 0x08;
            case ShaderStageType::TessControl: return 0x10;
            case ShaderStageType::TessEval:    return 0x20;
            default: return 0;
            }
        }

        enum class ShaderIOType {
            Float, Vec2, Vec3, Vec4,
            Int, IVec2, IVec3, IVec4,
            UInt, UVec2, UVec3, UVec4,
            Mat2, Mat3, Mat4,
            Unknown
        };

        struct ShaderIO {
            uint32_t     location;
            ShaderIOType type;
            std::string  name;
            bool         isInput;
            std::string  semantic;
        };

        struct ShaderModuleInfo {
            ShaderStageType        stage;
            std::string            path;
            std::vector<ShaderIO>  inputs;
        };

        enum class ShaderUniformType {
            Float, Vec2, Vec3, Vec4,
            Int, UInt,
            Mat2, Mat3, Mat4,

            Unknown
        };

        enum class SamplerType {
            Sampler2D,
            SamplerCube
        };

        struct ShaderUniformDesc {
            std::string      name;
            ShaderUniformType type;
            size_t           size;
            size_t           offset;
            uint32_t         set;
            uint32_t         binding;
            ShaderStageFlags stages;
        };

        struct ShaderSamplerDesc {
            std::string      name;
            uint32_t         set;
            uint32_t         binding;
            ShaderStageFlags stages;
        };

        struct ShaderPushConstantDesc {
            std::string      name;
            ShaderStageFlags stages;
            size_t           size;
            size_t           offset;
        };

        enum class DepthFunc {
            Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
        };

        enum class CullMode {
            None, Front, Back, FrontAndBack
        };

        enum class FillMode {
            Solid, Wireframe
        };

        enum class BlendMode {
            None, AlphaBlend, Additive, Multiply
        };

        enum class PrimitiveTopology {
            TriangleList, LineList, PointList
        };

        struct ShaderDescription {
            std::vector<ShaderModuleInfo>    modules;
            std::vector<ShaderUniformDesc>   globalUniforms;
            std::vector<ShaderUniformDesc>   objectUniforms;
            std::vector<ShaderSamplerDesc>   samplers;
            std::vector<ShaderPushConstantDesc> pushConstants;

            bool depthWriteEnable = true;
            bool depthTestEnable = true;
            DepthFunc depthFunc = DepthFunc::Less;

            CullMode cullMode = CullMode::Back;
            FillMode fillMode = FillMode::Solid;
            BlendMode blendMode = BlendMode::None;
            PrimitiveTopology topology = PrimitiveTopology::TriangleList;

            bool enableDynamicViewport = true;
            bool enableDynamicScissor = true;
            bool enableDynamicLineWidth = false;

            Framebuffer* framebuffer = nullptr;
        };

		virtual ~Shader() = default;

		static std::shared_ptr<Shader> Create(const ShaderDescription& desc);

		virtual bool UpdateGlobalState() = 0;
		virtual bool UpdateObject(Material* material) = 0;

		virtual bool AcquireResources(Material* material) = 0;
		virtual void ReleaseResources(Material* material) = 0;

		virtual void SetUniform(const std::string& name, void* data, size_t size) = 0;

        virtual void SetTexture(const std::string& name, Texture* texture, SamplerType type = SamplerType::Sampler2D) = 0;

		virtual void Use() = 0;
		virtual void Unuse() = 0;

		virtual void Reset() = 0;

        virtual void ApplyPipelineStates() = 0;
	};
}
