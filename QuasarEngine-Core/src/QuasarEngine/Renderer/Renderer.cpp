#include "qepch.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Renderer/Renderer2D.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/Mesh.h>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Tools/Math.h>

#include <QuasarEngine/UI/UIContainer.h>
#include <QuasarEngine/UI/UIText.h>
#include <QuasarEngine/UI/UIButton.h>
#include <QuasarEngine/UI/UITooltipLayer.h>
#include <QuasarEngine/UI/UISystem.h>
#include <QuasarEngine/UI/UITextInput.h>
#include <QuasarEngine/UI/UICheckbox.h>
#include <QuasarEngine/UI/UISlider.h>
#include <QuasarEngine/UI/UIProgressBar.h>
#include <QuasarEngine/UI/UIMenu.h>
#include <QuasarEngine/UI/UITabBar.h>

#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

namespace QuasarEngine
{
	void Renderer::Initialize()
	{
		auto extFor = [](RendererAPI::API api, Shader::ShaderStageType s) {
			if (api == RendererAPI::API::Vulkan) {
				switch (s) {
				case Shader::ShaderStageType::Vertex:     return ".vert.spv";
				case Shader::ShaderStageType::TessControl:return ".tesc.spv";
				case Shader::ShaderStageType::TessEval:   return ".tese.spv";
				case Shader::ShaderStageType::Fragment:   return ".frag.spv";
				default: return "";
				}
			}
			else {
				switch (s) {
				case Shader::ShaderStageType::Vertex:     return ".vert.glsl";
				case Shader::ShaderStageType::TessControl:return ".tesc.glsl";
				case Shader::ShaderStageType::TessEval:   return ".tese.glsl";
				case Shader::ShaderStageType::Fragment:   return ".frag.glsl";
				default: return "";
				}
			}
			};

		Shader::ShaderDescription desc;

		const auto api = RendererAPI::GetAPI();
		const std::string basePath = (api == RendererAPI::API::Vulkan)
			? "Assets/Shaders/vk/spv/"
			: "Assets/Shaders/gl/";

		const std::string name = "basic";

		std::string vertPath = basePath + name + extFor(api, Shader::ShaderStageType::Vertex);
		std::string fragPath = basePath + name + extFor(api, Shader::ShaderStageType::Fragment);

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				vertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				fragPath,
				{}
			}
		};

		struct alignas(16) GlobalUniforms
		{
			glm::mat4 view;
			glm::mat4 projection;
			glm::vec3 camera_position;

			int usePointLight;
			int useDirLight;

			int prefilterLevels;

			PointLight pointLights[4];
			DirectionalLight dirLights[4];
		};
		static_assert(offsetof(GlobalUniforms, pointLights) % 16 == 0, "pointLights offset must be 16-aligned");
		static_assert(offsetof(GlobalUniforms, dirLights) % 16 == 0, "dirLights offset must be 16-aligned");

		static_assert(sizeof(GlobalUniforms) % 16 == 0, "GlobalUniforms must be 16-aligned");

		constexpr Shader::ShaderStageFlags globalUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		desc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(GlobalUniforms, view), 0, 0, globalUniformsFlags},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(GlobalUniforms, projection), 0, 0, globalUniformsFlags},
			{"camera_position", Shader::ShaderUniformType::Vec3, sizeof(glm::vec3), offsetof(GlobalUniforms, camera_position), 0, 0, globalUniformsFlags},
			
			{"usePointLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(GlobalUniforms, usePointLight), 0, 0, globalUniformsFlags},
			{"useDirLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(GlobalUniforms, useDirLight), 0, 0, globalUniformsFlags},

			{"prefilterLevels", Shader::ShaderUniformType::Int, sizeof(int), offsetof(GlobalUniforms, prefilterLevels), 0, 0, globalUniformsFlags},
			
			{"pointLights", Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4, offsetof(GlobalUniforms, pointLights), 0, 0, globalUniformsFlags},
<<<<<<< HEAD
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(GlobalUniforms, dirLights), 0, 0, globalUniformsFlags},
=======
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(GlobalUniforms, dirLights), 0, 0, globalUniformsFlags}
>>>>>>> parent of 6e4f8d6 (Update)
		};

		struct alignas(16) ObjectUniforms {
			glm::mat4 model;

			glm::vec4 albedo;
			float roughness;
			float metallic;
			float ao;

			int has_albedo_texture;
			int has_normal_texture;
			int has_roughness_texture;
			int has_metallic_texture;
			int has_ao_texture;
		};

		static_assert(sizeof(ObjectUniforms) % 16 == 0, "ObjectUniforms must be 16-aligned");

		constexpr Shader::ShaderStageFlags objectUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		desc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(ObjectUniforms, model), 1, 0, objectUniformsFlags},

			{"albedo",	Shader::ShaderUniformType::Vec4, sizeof(glm::vec4), offsetof(ObjectUniforms, albedo), 1, 0, objectUniformsFlags},
			{"roughness",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, roughness), 1, 0, objectUniformsFlags},
			{"metallic",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, metallic), 1, 0, objectUniformsFlags},
			{"ao",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, ao), 1, 0, objectUniformsFlags},

			{"has_albedo_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_albedo_texture), 1, 0, objectUniformsFlags},
			{"has_normal_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_normal_texture), 1, 0, objectUniformsFlags},
			{"has_roughness_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_roughness_texture), 1, 0, objectUniformsFlags},
			{"has_metallic_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_metallic_texture), 1, 0, objectUniformsFlags},
			{"has_ao_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_ao_texture), 1, 0, objectUniformsFlags},
		};

		desc.samplers = {
			{"albedo_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture", 1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture", 1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)},

			{"irradiance_map", 1, 6, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"prefilter_map", 1, 7, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
<<<<<<< HEAD
			{"brdf_lut", 1, 8, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
=======
			{"brdf_lut", 1, 8, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
>>>>>>> parent of 6e4f8d6 (Update)
		};

		desc.blendMode = Shader::BlendMode::None;
		desc.cullMode = Shader::CullMode::Back;
		desc.fillMode = Shader::FillMode::Solid;
		desc.depthFunc = Shader::DepthFunc::Less;
		desc.depthTestEnable = true;
		desc.depthWriteEnable = true;
		desc.topology = Shader::PrimitiveTopology::TriangleList;
		desc.enableDynamicViewport = true;
		desc.enableDynamicScissor = true;
		desc.enableDynamicLineWidth = false;

		m_SceneData.m_Shader = Shader::Create(desc);

		/*Shader::ShaderDescription phyDebDesc;

		std::string phyDebVertExt;
		std::string phyDebFragExt;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			phyDebVertExt = ".vert.spv";
			phyDebFragExt = ".frag.spv";
		}
		else
		{
			phyDebVertExt = ".vert.glsl";
			phyDebFragExt = ".frag.glsl";
		}

		std::string phyDebVertPath = basePath + "debug" + phyDebVertExt;
		std::string phyDebFragPath = basePath + "debug" + phyDebFragExt;

		phyDebDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				phyDebVertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inColor",    true, ""},
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				phyDebFragPath,
				{}
			}
		};

		struct alignas(16) PhyDebGlobalUniforms
		{
			glm::mat4 view;
			glm::mat4 projection;
		};

		constexpr Shader::ShaderStageFlags phyDebGlobalUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		phyDebDesc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebGlobalUniforms, view), 0, 0, phyDebGlobalUniformsFlags},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebGlobalUniforms, projection), 0, 0, phyDebGlobalUniformsFlags}
		};

		struct PhyDebObjectUniforms {
			glm::mat4 model;
		};

		constexpr Shader::ShaderStageFlags phyDebObjectUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		phyDebDesc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebObjectUniforms, model), 1, 0, phyDebObjectUniformsFlags},
		};

		phyDebDesc.samplers = {
			
		};

		phyDebDesc.blendMode = Shader::BlendMode::None;
		phyDebDesc.cullMode = Shader::CullMode::None;
		phyDebDesc.fillMode = Shader::FillMode::Solid;
		phyDebDesc.depthFunc = Shader::DepthFunc::Always;
		phyDebDesc.depthTestEnable = false;
		phyDebDesc.depthWriteEnable = false;
		phyDebDesc.topology = Shader::PrimitiveTopology::LineList;
		phyDebDesc.enableDynamicViewport = true;
		phyDebDesc.enableDynamicScissor = true;
		phyDebDesc.enableDynamicLineWidth = false;

		m_SceneData.m_PhysicDebugShader = Shader::Create(phyDebDesc);*/

		Shader::ShaderDescription terrainDesc;

		terrainDesc.cullMode = Shader::CullMode::Back;

		const std::string tName = "gpuheight";

		std::string tVertPath = basePath + tName + extFor(api, Shader::ShaderStageType::Vertex);
		std::string tTcsPath = basePath + tName + extFor(api, Shader::ShaderStageType::TessControl);
		std::string tTesPath = basePath + tName + extFor(api, Shader::ShaderStageType::TessEval);
		std::string tFragPath = basePath + tName + extFor(api, Shader::ShaderStageType::Fragment);

		terrainDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				tVertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal", true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::TessControl,
				tTcsPath,
				{}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::TessEval,
				tTesPath,
				{}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				tFragPath,
				{}
			}
		};

		struct alignas(16) TerrainGlobalUniforms {
			glm::mat4 view;
			glm::mat4 projection;
			glm::vec3 camera_position;

			int usePointLight;
			int useDirLight;

			PointLight pointLights[4];
			DirectionalLight dirLights[4];
		};
		static_assert(offsetof(TerrainGlobalUniforms, pointLights) % 16 == 0, "pointLights offset must be 16-aligned");
		static_assert(offsetof(TerrainGlobalUniforms, dirLights) % 16 == 0, "dirLights offset must be 16-aligned");

		constexpr Shader::ShaderStageFlags TerrainGlobalStages =
			Shader::StageToBit(Shader::ShaderStageType::Vertex) |
			Shader::StageToBit(Shader::ShaderStageType::TessControl) |
			Shader::StageToBit(Shader::ShaderStageType::TessEval) |
			Shader::StageToBit(Shader::ShaderStageType::Fragment);

		terrainDesc.globalUniforms = {
			{"view",            Shader::ShaderUniformType::Mat4, sizeof(glm::mat4),                     offsetof(TerrainGlobalUniforms, view),            0, 0, TerrainGlobalStages},
			{"projection",      Shader::ShaderUniformType::Mat4, sizeof(glm::mat4),                     offsetof(TerrainGlobalUniforms, projection),      0, 0, TerrainGlobalStages},
			{"camera_position", Shader::ShaderUniformType::Vec3, sizeof(glm::vec3),                     offsetof(TerrainGlobalUniforms, camera_position), 0, 0, TerrainGlobalStages},

			{"usePointLight",   Shader::ShaderUniformType::Int,  sizeof(int),                           offsetof(TerrainGlobalUniforms, usePointLight),   0, 0, TerrainGlobalStages},
			{"useDirLight",     Shader::ShaderUniformType::Int,  sizeof(int),                           offsetof(TerrainGlobalUniforms, useDirLight),     0, 0, TerrainGlobalStages},

			{"pointLights",     Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4,             offsetof(TerrainGlobalUniforms, pointLights),     0, 0, TerrainGlobalStages},
			{"dirLights",       Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4,       offsetof(TerrainGlobalUniforms, dirLights),       0, 0, TerrainGlobalStages},
		};

		struct alignas(16) TerrainObjectUniforms {
			glm::mat4 model;

			glm::vec4 albedo;
			float roughness;
			float metallic;
			float ao;

			int has_albedo_texture;
			int has_normal_texture;
			int has_roughness_texture;
			int has_metallic_texture;
			int has_ao_texture;

			float heightMult;
			int   uTextureScale;
		};

		constexpr Shader::ShaderStageFlags TOFlags =
			Shader::StageToBit(Shader::ShaderStageType::Vertex) |
			Shader::StageToBit(Shader::ShaderStageType::TessControl) |
			Shader::StageToBit(Shader::ShaderStageType::TessEval) |
			Shader::StageToBit(Shader::ShaderStageType::Fragment);

		terrainDesc.objectUniforms = {
			{"model",               Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(TerrainObjectUniforms, model),               1, 0, TOFlags},

			{"albedo",              Shader::ShaderUniformType::Vec4,  sizeof(glm::vec4),  offsetof(TerrainObjectUniforms, albedo),              1, 0, TOFlags},
			{"roughness",           Shader::ShaderUniformType::Float, sizeof(float),      offsetof(TerrainObjectUniforms, roughness),           1, 0, TOFlags},
			{"metallic",            Shader::ShaderUniformType::Float, sizeof(float),      offsetof(TerrainObjectUniforms, metallic),            1, 0, TOFlags},
			{"ao",                  Shader::ShaderUniformType::Float, sizeof(float),      offsetof(TerrainObjectUniforms, ao),                  1, 0, TOFlags},

			{"has_albedo_texture",  Shader::ShaderUniformType::Int,   sizeof(int),        offsetof(TerrainObjectUniforms, has_albedo_texture),  1, 0, TOFlags},
			{"has_normal_texture",  Shader::ShaderUniformType::Int,   sizeof(int),        offsetof(TerrainObjectUniforms, has_normal_texture),  1, 0, TOFlags},
			{"has_roughness_texture",Shader::ShaderUniformType::Int,  sizeof(int),        offsetof(TerrainObjectUniforms, has_roughness_texture),1, 0, TOFlags},
			{"has_metallic_texture",Shader::ShaderUniformType::Int,   sizeof(int),        offsetof(TerrainObjectUniforms, has_metallic_texture),1, 0, TOFlags},
			{"has_ao_texture",      Shader::ShaderUniformType::Int,   sizeof(int),        offsetof(TerrainObjectUniforms, has_ao_texture),      1, 0, TOFlags},

			{"heightMult",          Shader::ShaderUniformType::Float, sizeof(float),      offsetof(TerrainObjectUniforms, heightMult),          1, 0, TOFlags},
			{"uTextureScale",       Shader::ShaderUniformType::Int,   sizeof(int),        offsetof(TerrainObjectUniforms, uTextureScale),       1, 0, TOFlags},
		};

		terrainDesc.samplers = {
			{"heightMap",        1, 1, Shader::StageToBit(Shader::ShaderStageType::TessEval)},

			{"albedo_texture",   1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture",   1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture",1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture",       1, 6, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
		};

		terrainDesc.blendMode = Shader::BlendMode::None;
		terrainDesc.cullMode = Shader::CullMode::Back;
		terrainDesc.fillMode = Shader::FillMode::Solid;
		terrainDesc.depthFunc = Shader::DepthFunc::Less;
		terrainDesc.depthTestEnable = true;
		terrainDesc.depthWriteEnable = true;

		terrainDesc.topology = Shader::PrimitiveTopology::PatchList;

		terrainDesc.patchControlPoints = 4;

		terrainDesc.enableDynamicViewport = true;
		terrainDesc.enableDynamicScissor = true;

		m_SceneData.m_TerrainShader = Shader::Create(terrainDesc);

		Shader::ShaderDescription skinnedDesc;

		const std::string sName = "basic_anim";

		std::string sVertPath = basePath + sName + extFor(api, Shader::ShaderStageType::Vertex);
		std::string sFragPath = basePath + sName + extFor(api, Shader::ShaderStageType::Fragment);

		skinnedDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				sVertPath,
				{
					{0, Shader::ShaderIOType::Vec3,  "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3,  "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2,  "inTexCoord", true, ""},
					{3, Shader::ShaderIOType::IVec4, "inBoneIds",  true, ""},
					{4, Shader::ShaderIOType::Vec4,  "inWeights",  true, ""},
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				sFragPath,
				{}
			}
		};

		struct alignas(16) SkinnedGlobalUniforms {
			glm::mat4 view;
			glm::mat4 projection;
			glm::vec3 camera_position;

			int usePointLight;
			int useDirLight;

			int prefilterLevels;

			PointLight pointLights[4];
			DirectionalLight dirLights[4];
		};

		constexpr Shader::ShaderStageFlags SkinnedGlobalStages = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		skinnedDesc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedGlobalUniforms, view), 0, 0, SkinnedGlobalStages},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedGlobalUniforms, projection), 0, 0, SkinnedGlobalStages},
			{"camera_position", Shader::ShaderUniformType::Vec3, sizeof(glm::vec3), offsetof(SkinnedGlobalUniforms, camera_position), 0, 0, SkinnedGlobalStages},

			{"usePointLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(SkinnedGlobalUniforms, usePointLight), 0, 0, SkinnedGlobalStages},
			{"useDirLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(SkinnedGlobalUniforms, useDirLight), 0, 0, SkinnedGlobalStages},

			{"prefilterLevels", Shader::ShaderUniformType::Int, sizeof(int), offsetof(SkinnedGlobalUniforms, prefilterLevels), 0, 0, SkinnedGlobalStages},

			{"pointLights", Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4, offsetof(SkinnedGlobalUniforms, pointLights), 0, 0, SkinnedGlobalStages},
<<<<<<< HEAD
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(SkinnedGlobalUniforms, dirLights), 0, 0, SkinnedGlobalStages}
=======
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(SkinnedGlobalUniforms, dirLights), 0, 0, SkinnedGlobalStages},
>>>>>>> parent of 6e4f8d6 (Update)
		};

		struct alignas(16) SkinnedObjectUniforms {
			glm::mat4 model;

			glm::vec4 albedo;
			float roughness;
			float metallic;
			float ao;

			int has_albedo_texture;
			int has_normal_texture;
			int has_roughness_texture;
			int has_metallic_texture;
			int has_ao_texture;

			glm::mat4 finalBonesMatrices[QE_MAX_BONES];
		};

		skinnedDesc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedObjectUniforms, model), 1, 0, SkinnedGlobalStages},

			{"albedo",	Shader::ShaderUniformType::Vec4, sizeof(glm::vec4), offsetof(SkinnedObjectUniforms, albedo), 1, 0, SkinnedGlobalStages},
			{"roughness",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, roughness), 1, 0, SkinnedGlobalStages},
			{"metallic",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, metallic), 1, 0, SkinnedGlobalStages},
			{"ao",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, ao), 1, 0, SkinnedGlobalStages},

			{"has_albedo_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_albedo_texture), 1, 0, SkinnedGlobalStages},
			{"has_normal_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_normal_texture), 1, 0, SkinnedGlobalStages},
			{"has_roughness_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_roughness_texture), 1, 0, SkinnedGlobalStages},
			{"has_metallic_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_metallic_texture), 1, 0, SkinnedGlobalStages},
			{"has_ao_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_ao_texture), 1, 0, SkinnedGlobalStages},

			{"finalBonesMatrices",  Shader::ShaderUniformType::Unknown,sizeof(glm::mat4) * QE_MAX_BONES, offsetof(SkinnedObjectUniforms, finalBonesMatrices),  1, 0, SkinnedGlobalStages},
		};

		skinnedDesc.samplers = {
			{"albedo_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture", 1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture", 1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
<<<<<<< HEAD
			{"ao_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)},

			{"irradiance_map", 1, 6, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"prefilter_map", 1, 7, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"brdf_lut", 1, 8, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
=======
			{"ao_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
>>>>>>> parent of 6e4f8d6 (Update)
		};

		skinnedDesc.blendMode = Shader::BlendMode::None;
		skinnedDesc.cullMode = Shader::CullMode::Back;
		skinnedDesc.fillMode = Shader::FillMode::Solid;
		skinnedDesc.depthFunc = Shader::DepthFunc::Less;
		skinnedDesc.depthTestEnable = true;
		skinnedDesc.depthWriteEnable = true;
		skinnedDesc.topology = Shader::PrimitiveTopology::TriangleList;
		skinnedDesc.enableDynamicViewport = true;
		skinnedDesc.enableDynamicScissor = true;

		m_SceneData.m_SkinnedShader = Shader::Create(skinnedDesc);

		for (int i = 0; i < QE_MAX_BONES; ++i)
			m_SceneData.m_IdentityBones[i] = glm::mat4(1.0f);

		/*m_SceneData.m_Skybox = BasicSkybox::CreateBasicSkybox();

		m_SceneData.m_Skybox->LoadCubemap({
			"Assets/Textures/Skybox/right.jpg",   // +X
			"Assets/Textures/Skybox/left.jpg",    // -X
			"Assets/Textures/Skybox/top.jpg",     // +Y
			"Assets/Textures/Skybox/bottom.jpg",  // -Y
			"Assets/Textures/Skybox/front.jpg",   // +Z
			"Assets/Textures/Skybox/back.jpg"     // -Z
		});*/

		SkyboxHDR::Settings skyboxSettings;
		skyboxSettings.hdrPath = "Assets/HDR/kloofendal_48d_partly_cloudy_puresky_4k.hdr";
		m_SceneData.m_SkyboxHDR = std::make_shared<SkyboxHDR>(skyboxSettings);

		m_SceneData.m_ScriptSystem = std::make_unique<ScriptSystem>();
		m_SceneData.m_ScriptSystem->Initialize();

		m_SceneData.m_PointsBuffer.fill(PointLight());
		m_SceneData.m_DirectionalsBuffer.fill(DirectionalLight());

		m_SceneData.m_UI = std::make_unique<UISystem>();
<<<<<<< HEAD
=======
		
		struct UISharedState {
			int quality = 1;
		};
		auto state = std::make_shared<UISharedState>();
		
		auto root = std::make_shared<UIContainer>("Root");
		root->layout = UILayoutType::Vertical;
		root->gap = 10.f;
		root->Style().padding = 12.f;
		root->Style().bg = { 0.10f, 0.10f, 0.12f, 0.85f };
		root->Transform().pos = { 10.f, 10.f };
		root->Transform().size = { 720.f, 0.f };
		
		{
			auto title = std::make_shared<UIText>("Title");
			title->text = "UI Playground - QuasarEngine";
			title->Style().bg = { 0,0,0,0 };
			title->Style().fg = { 0.95f, 0.96f, 0.99f, 1 };
			title->Transform().size = { 0.f, 36.f };
			root->AddChild(title);
		}
		
		auto row1 = std::make_shared<UIContainer>("Row1");
		row1->layout = UILayoutType::Horizontal;
		row1->gap = 12.f;
		row1->Style().bg = { 0,0,0,0 };
		root->AddChild(row1);

		auto colLeft = std::make_shared<UIContainer>("ColLeft");
		colLeft->layout = UILayoutType::Vertical;
		colLeft->gap = 6.f;
		colLeft->Style().padding = 8.f;
		colLeft->Style().bg = { 0.12f,0.13f,0.16f,1.f };
		colLeft->Transform().size = { 340.f, 0.f };
		row1->AddChild(colLeft);

		{
			auto btnResume = std::make_shared<UIButton>("BtnResume");
			btnResume->label = "Reprendre";
			btnResume->SetTabIndex(0);
			btnResume->onClick = []() { std::cout << "[UI] Resume clicked\n"; };
			colLeft->AddChild(btnResume);

			auto btnOptions = std::make_shared<UIButton>("BtnOptions");
			btnOptions->label = "Options";
			btnOptions->SetTabIndex(1);
			btnOptions->onClick = []() { std::cout << "[UI] Options clicked\n"; };
			colLeft->AddChild(btnOptions);

			auto btnQuit = std::make_shared<UIButton>("BtnQuit");
			btnQuit->label = "Quitter";
			btnQuit->SetTabIndex(2);
			btnQuit->onClick = []() { std::cout << "[UI] Quit clicked\n"; };
			colLeft->AddChild(btnQuit);
		}

		{
			auto cbVsync = std::make_shared<UICheckbox>("CbVsync");
			cbVsync->label = "VSync";
			cbVsync->SetTabIndex(3);
			colLeft->AddChild(cbVsync);

			auto cbPost = std::make_shared<UICheckbox>("CbPostFX");
			cbPost->label = "Post-Processing";
			cbPost->SetTabIndex(4);
			colLeft->AddChild(cbPost);

			auto cbDisabled = std::make_shared<UICheckbox>("CbDisableResume");
			cbDisabled->label = "Desactiver le bouton Reprendre (visuel)";
			cbDisabled->SetTabIndex(5);
			
			struct SyncDisable : UIElement {
				UIButton* target; UICheckbox* source;
				explicit SyncDisable(std::string id, UIButton* t, UICheckbox* s) : UIElement(std::move(id)), target(t), source(s) {}
				void Measure(UILayoutContext&) override { Transform().size = { 0,0 }; }
				void BuildDraw(UIRenderContext&) override {
					if (target && source) target->SetEnabled(!source->checked);
				}
			};
			
			colLeft->AddChild(cbDisabled);
			colLeft->AddChild(std::make_shared<SyncDisable>("SyncDisableResume",
				static_cast<UIButton*>(colLeft->Children()[0].get()),
				cbDisabled.get()));
		}

		{
			auto rLow = std::make_shared<UIRadioButton>("RadioLow");
			rLow->label = "Qualite : Basse";
			rLow->groupValue = &state->quality; rLow->index = 0;
			rLow->SetTabIndex(6);
			colLeft->AddChild(rLow);

			auto rMed = std::make_shared<UIRadioButton>("RadioMed");
			rMed->label = "Qualite : Moyenne";
			rMed->groupValue = &state->quality; rMed->index = 1;
			rMed->SetTabIndex(7);
			colLeft->AddChild(rMed);

			auto rHigh = std::make_shared<UIRadioButton>("RadioHigh");
			rHigh->label = "Qualite : Haute";
			rHigh->groupValue = &state->quality; rHigh->index = 2;
			rHigh->SetTabIndex(8);
			colLeft->AddChild(rHigh);
		}

		auto colRight = std::make_shared<UIContainer>("ColRight");
		colRight->layout = UILayoutType::Vertical;
		colRight->gap = 6.f;
		colRight->Style().padding = 8.f;
		colRight->Style().bg = { 0.12f,0.13f,0.16f,1.f };
		colRight->Transform().size = { 340.f, 0.f };
		row1->AddChild(colRight);

		std::shared_ptr<UISlider> sMaster, sSpeed;
		{
			auto lblVol = std::make_shared<UIText>("LblVolume"); lblVol->text = "Volume principal";
			colRight->AddChild(lblVol);

			sMaster = std::make_shared<UISlider>("SliderMaster");
			sMaster->min = 0; sMaster->max = 100; sMaster->value = 25;
			sMaster->SetTabIndex(9);
			colRight->AddChild(sMaster);

			auto lblSpeed = std::make_shared<UIText>("LblSpeed"); lblSpeed->text = "Vitesse";
			colRight->AddChild(lblSpeed);

			sSpeed = std::make_shared<UISlider>("SliderSpeed");
			sSpeed->min = 0; sSpeed->max = 10; sSpeed->value = 3.5f;
			sSpeed->SetTabIndex(10);
			colRight->AddChild(sSpeed);
		}

		{
			auto pb = std::make_shared<UIProgressBar>("ProgressLoad");
			pb->value = 0.25f;
			colRight->AddChild(pb);

			struct SyncProgress : UIElement {
				UIProgressBar* bar; UISlider* src;
				explicit SyncProgress(std::string id, UIProgressBar* b, UISlider* s) : UIElement(std::move(id)), bar(b), src(s) {}
				void Measure(UILayoutContext&) override { Transform().size = { 0,0 }; }
				void BuildDraw(UIRenderContext&) override {
					if (bar && src) bar->value = (src->value - src->min) / std::max(0.0001f, (src->max - src->min));
				}
			};
			colRight->AddChild(std::make_shared<SyncProgress>("SyncPB", pb.get(), sMaster.get()));
		}

		{
			auto lblName = std::make_shared<UIText>("LblName"); lblName->text = "Nom du projet";
			colRight->AddChild(lblName);

			auto inpName = std::make_shared<UITextInput>("InpName");
			inpName->text = "QuasarGame";
			inpName->SetTabIndex(11);
			colRight->AddChild(inpName);

			auto lblRO = std::make_shared<UIText>("LblRO"); lblRO->text = "Champ en lecture seule (disabled):";
			colRight->AddChild(lblRO);

			auto inpRO = std::make_shared<UITextInput>("InpRO");
			inpRO->text = "Indisponible";
			inpRO->SetEnabled(false);
			colRight->AddChild(inpRO);
		}
		
		auto tabs = std::make_shared<UITabs>("Tabs");
		tabs->Style().bg = { 0.11f,0.11f,0.13f,1 };
		tabs->Transform().size = { 0.f, 260.f };
		tabs->tabbar->labels = { "General", "Audio", "À propos" };
		root->AddChild(tabs);

		{
			auto gen = std::make_shared<UIContainer>("TabGeneral");
			gen->layout = UILayoutType::Vertical; gen->gap = 6.f;
			{
				auto t = std::make_shared<UIText>("TGen"); t->text = "Parametres generaux";
				gen->AddChild(t);

				auto cb = std::make_shared<UICheckbox>("GenCB"); cb->label = "Activer HUD";
				gen->AddChild(cb);
				auto cb2 = std::make_shared<UICheckbox>("GenCB2"); cb2->label = "Afficher FPS";
				gen->AddChild(cb2);
			}
			tabs->AddChild(gen);
		}

		{
			auto aud = std::make_shared<UIContainer>("TabAudio");
			aud->layout = UILayoutType::Vertical; aud->gap = 6.f;
			{
				auto t = std::make_shared<UIText>("TAud"); t->text = "Options audio";
				aud->AddChild(t);

				auto sfx = std::make_shared<UISlider>("SFX"); sfx->min = 0; sfx->max = 100; sfx->value = 70;
				aud->AddChild(sfx);
				auto mus = std::make_shared<UISlider>("Music"); mus->min = 0; mus->max = 100; mus->value = 40;
				aud->AddChild(mus);
			}
			tabs->AddChild(aud);
		}

		{
			auto about = std::make_shared<UIContainer>("TabAbout");
			about->layout = UILayoutType::Vertical; about->gap = 6.f;
			{
				auto t1 = std::make_shared<UIText>("Tab1"); t1->text = "QuasarEngine UI Sandbox";
				about->AddChild(t1);
				auto t2 = std::make_shared<UIText>("Tab2"); t2->text = "Test de widgets, layout, focus/tab, disabled, etc.";
				about->AddChild(t2);
			}
			tabs->AddChild(about);
		}

		auto menu = std::make_shared<UIMenu>("CtxMenu");
		menu->items = {
			{ "Nouveau",  false, []() { std::cout << "[Menu] Nouveau\n"; } },
			{ "Ouvrir...",false, []() { std::cout << "[Menu] Ouvrir\n"; } },
			{ "Enregistrer",false, []() { std::cout << "[Menu] Enregistrer\n"; } },
			{ "Quitter",  false, []() { std::cout << "[Menu] Quitter\n"; } }
		};
		root->AddChild(menu);

		{
			auto openMenuBtn = std::make_shared<UIButton>("BtnOpenMenu");
			openMenuBtn->label = "Ouvrir le menu";
			openMenuBtn->SetTabIndex(12);
			openMenuBtn->onClick = [menu, openMenuBtn]() {
				const Rect r = openMenuBtn->Transform().rect;
				menu->OpenAt(r.x, r.y + r.h + 4.f);
				};
			root->AddChild(openMenuBtn);
		}

		auto tooltip = std::make_shared<UITooltipLayer>("Tooltip");
		root->AddChild(tooltip);

		tooltip->Show("Astuce: Tab pour naviguer, Espace/Entree pour activer.", 28.f, 80.f);

		m_SceneData.m_UI->SetRoot(root);
>>>>>>> parent of 6e4f8d6 (Update)
	}

	void Renderer::Shutdown()
	{
		//m_SceneData.m_Skybox.reset();
		m_SceneData.m_SkyboxHDR.reset();
		m_SceneData.m_Shader.reset();
		//m_SceneData.m_PhysicDebugShader.reset();
		m_SceneData.m_SkinnedShader.reset();
		m_SceneData.m_TerrainShader.reset();
		m_SceneData.m_UI.reset();
		m_SceneData.m_ScriptSystem.reset();
	}

	void Renderer::BeginScene(Scene& scene)
	{
		m_SceneData.m_Scene = &scene;
	}

	void Renderer::Render(BaseCamera& camera)
<<<<<<< HEAD
	{		
=======
	{
>>>>>>> parent of 6e4f8d6 (Update)
		auto FindAnimatorForEntity = [&](Entity e) -> AnimationComponent*
			{
				if (e.HasComponent<AnimationComponent>())
					return &e.GetComponent<AnimationComponent>();

				if (e.HasComponent<HierarchyComponent>()) {
					const auto& h = e.GetComponent<HierarchyComponent>();
					if (h.m_Parent != UUID::Null()) {
						if (auto parentOpt = m_SceneData.m_Scene->GetEntityByUUID(h.m_Parent)) {
							if (parentOpt->HasComponent<AnimationComponent>())
								return &parentOpt->GetComponent<AnimationComponent>();
						}
					}
				}
				return nullptr;
			};

		const glm::mat4 VP = camera.getProjectionMatrix() * camera.getViewMatrix();
		Math::Frustum frustum = Math::CalculateFrustum(VP);

		glm::mat4 viewMatrix = camera.getViewMatrix();
		glm::mat4 projectionMatrix = camera.getProjectionMatrix();

		m_SceneData.m_Shader->Use();

		//int totalEntity = 0;
		//int entityDraw = 0;

		m_SceneData.m_Shader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_Shader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));
		m_SceneData.m_Shader->SetUniform("camera_position", &camera.GetPosition(), sizeof(glm::vec3));

		m_SceneData.m_Shader->SetUniform("prefilterLevels", &m_SceneData.m_SkyboxHDR->GetSettings().prefilterMipLevels, sizeof(int));

<<<<<<< HEAD
=======
		m_SceneData.nDirs = 0;
		m_SceneData.nPts = 0;

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<LightComponent, TransformComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry()};

			auto& lc = entity.GetComponent<LightComponent>();
			auto& tr = entity.GetComponent<TransformComponent>();

			if (lc.lightType == LightComponent::LightType::DIRECTIONAL && m_SceneData.nDirs < 4)
			{
				DirectionalLight dl = lc.directional_light;
				dl.direction = -Math::ForwardFromEulerRad(tr.Rotation);
				m_SceneData.m_DirectionalsBuffer[m_SceneData.nDirs++] = dl;
			}
			else if (lc.lightType == LightComponent::LightType::POINT && m_SceneData.nPts < 4)
			{
				PointLight pl = lc.point_light;
				pl.position = tr.Position;
				m_SceneData.m_PointsBuffer[m_SceneData.nPts++] = pl;
			}
		}

>>>>>>> parent of 6e4f8d6 (Update)
		m_SceneData.m_Shader->SetUniform("usePointLight", &m_SceneData.nPts, sizeof(int));
		m_SceneData.m_Shader->SetUniform("useDirLight", &m_SceneData.nDirs, sizeof(int));

		m_SceneData.m_Shader->SetUniform("pointLights", m_SceneData.m_PointsBuffer.data(), sizeof(PointLight) * 4);
		m_SceneData.m_Shader->SetUniform("dirLights", m_SceneData.m_DirectionalsBuffer.data(), sizeof(DirectionalLight) * 4);

		if (!m_SceneData.m_Shader->UpdateGlobalState())
		{
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, MeshComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& mc = entity.GetComponent<MeshComponent>();
			auto& matc = entity.GetComponent<MaterialComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			if (!mr.m_Rendered || !mc.HasMesh()) continue;

			if (mc.GetMesh().HasSkinning()) continue;

			glm::mat4 model = tr.GetGlobalTransform();
			if (mc.HasLocalNodeTransform()) model *= mc.GetLocalNodeTransform();

			if (!mc.GetMesh().IsVisible(frustum, model)) {
				// continue;
			}

			Material& material = matc.GetMaterial();

			m_SceneData.m_Shader->SetUniform("model", &model, sizeof(glm::mat4));

			m_SceneData.m_Shader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
			float rough = material.GetRoughness();
			float metal = material.GetMetallic();
			float ao = material.GetAO();
			m_SceneData.m_Shader->SetUniform("roughness", &rough, sizeof(float));
			m_SceneData.m_Shader->SetUniform("metallic", &metal, sizeof(float));
			m_SceneData.m_Shader->SetUniform("ao", &ao, sizeof(float));

			int hasA = material.HasTexture(TextureType::Albedo) ? 1 : 0;
			int hasN = material.HasTexture(TextureType::Normal) ? 1 : 0;
			int hasR = material.HasTexture(TextureType::Roughness) ? 1 : 0;
			int hasM = material.HasTexture(TextureType::Metallic) ? 1 : 0;
			int hasO = material.HasTexture(TextureType::AO) ? 1 : 0;

			m_SceneData.m_Shader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_normal_texture", &hasN, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_ao_texture", &hasO, sizeof(int));

			m_SceneData.m_Shader->SetTexture("albedo_texture", material.GetTexture(TextureType::Albedo));
			m_SceneData.m_Shader->SetTexture("normal_texture", material.GetTexture(TextureType::Normal));
			m_SceneData.m_Shader->SetTexture("roughness_texture", material.GetTexture(TextureType::Roughness));
			m_SceneData.m_Shader->SetTexture("metallic_texture", material.GetTexture(TextureType::Metallic));
			m_SceneData.m_Shader->SetTexture("ao_texture", material.GetTexture(TextureType::AO));

			m_SceneData.m_Shader->SetTexture("irradiance_map", m_SceneData.m_SkyboxHDR->GetIrradianceMap().get());
			m_SceneData.m_Shader->SetTexture("prefilter_map", m_SceneData.m_SkyboxHDR->GetPrefilterMap().get());
			m_SceneData.m_Shader->SetTexture("brdf_lut", m_SceneData.m_SkyboxHDR->GetBrdfLUT().get());

			if (!m_SceneData.m_Shader->UpdateObject(&material)) continue;

			mc.GetMesh().draw();

			m_SceneData.m_Shader->Reset();
		}

		m_SceneData.m_Shader->Unuse();

		m_SceneData.m_SkinnedShader->Use();

		m_SceneData.m_SkinnedShader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_SkinnedShader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));
		m_SceneData.m_SkinnedShader->SetUniform("camera_position", &camera.GetPosition(), sizeof(glm::vec3));

		m_SceneData.m_SkinnedShader->SetUniform("usePointLight", &m_SceneData.nPts, sizeof(int));
		m_SceneData.m_SkinnedShader->SetUniform("useDirLight", &m_SceneData.nDirs, sizeof(int));

		m_SceneData.m_SkinnedShader->SetUniform("prefilterLevels", &m_SceneData.m_SkyboxHDR->GetSettings().prefilterMipLevels, sizeof(int));

		m_SceneData.m_SkinnedShader->SetUniform("pointLights", m_SceneData.m_PointsBuffer.data(), sizeof(PointLight) * 4);
		m_SceneData.m_SkinnedShader->SetUniform("dirLights", m_SceneData.m_DirectionalsBuffer.data(), sizeof(DirectionalLight) * 4);

		if (!m_SceneData.m_SkinnedShader->UpdateGlobalState())
		{
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, MeshComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& mc = entity.GetComponent<MeshComponent>();
			auto& matc = entity.GetComponent<MaterialComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			if (!mr.m_Rendered || !mc.HasMesh()) continue;

			if (!mc.GetMesh().HasSkinning()) continue;

			glm::mat4 model = tr.GetGlobalTransform();
			if (mc.HasLocalNodeTransform()) model *= mc.GetLocalNodeTransform();

			if (!mc.GetMesh().IsVisible(frustum, model)) {
				// continue;
			}

			m_SceneData.m_SkinnedShader->SetUniform("model", &model, sizeof(glm::mat4));

			Material& material = matc.GetMaterial();

			m_SceneData.m_SkinnedShader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
			float rough = material.GetRoughness();
			float metal = material.GetMetallic();
			float ao = material.GetAO();
			m_SceneData.m_SkinnedShader->SetUniform("roughness", &rough, sizeof(float));
			m_SceneData.m_SkinnedShader->SetUniform("metallic", &metal, sizeof(float));
			m_SceneData.m_SkinnedShader->SetUniform("ao", &ao, sizeof(float));

			int hasA = material.HasTexture(TextureType::Albedo) ? 1 : 0;
			int hasN = material.HasTexture(TextureType::Normal) ? 1 : 0;
			int hasR = material.HasTexture(TextureType::Roughness) ? 1 : 0;
			int hasM = material.HasTexture(TextureType::Metallic) ? 1 : 0;
			int hasO = material.HasTexture(TextureType::AO) ? 1 : 0;

			m_SceneData.m_SkinnedShader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_normal_texture", &hasN, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_ao_texture", &hasO, sizeof(int));

			m_SceneData.m_SkinnedShader->SetTexture("albedo_texture", material.GetTexture(TextureType::Albedo));
			m_SceneData.m_SkinnedShader->SetTexture("normal_texture", material.GetTexture(TextureType::Normal));
			m_SceneData.m_SkinnedShader->SetTexture("roughness_texture", material.GetTexture(TextureType::Roughness));
			m_SceneData.m_SkinnedShader->SetTexture("metallic_texture", material.GetTexture(TextureType::Metallic));
			m_SceneData.m_SkinnedShader->SetTexture("ao_texture", material.GetTexture(TextureType::AO));

			m_SceneData.m_SkinnedShader->SetTexture("irradiance_map", m_SceneData.m_SkyboxHDR->GetIrradianceMap().get());
			m_SceneData.m_SkinnedShader->SetTexture("prefilter_map", m_SceneData.m_SkyboxHDR->GetPrefilterMap().get());
			m_SceneData.m_SkinnedShader->SetTexture("brdf_lut", m_SceneData.m_SkyboxHDR->GetBrdfLUT().get());

			const AnimationComponent* anim = FindAnimatorForEntity(entity);
			if (anim && !anim->GetFinalBoneMatrices().empty()) {
				std::vector<glm::mat4> mats = anim->GetFinalBoneMatrices();
				const size_t n = std::min(mats.size(), (size_t)QE_MAX_BONES);
				if (n == (size_t)QE_MAX_BONES) {
					m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices", mats.data(), sizeof(glm::mat4) * QE_MAX_BONES);
				}
				else {
					std::array<glm::mat4, QE_MAX_BONES> tmp = m_SceneData.m_IdentityBones;
					std::memcpy(tmp.data(), mats.data(), sizeof(glm::mat4) * n);
					m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices", tmp.data(), sizeof(glm::mat4) * QE_MAX_BONES);
				}
			}
			else {
				m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices",
					m_SceneData.m_IdentityBones.data(),
					sizeof(glm::mat4) * QE_MAX_BONES);
			}

			if (!m_SceneData.m_SkinnedShader->UpdateObject(&material)) continue;

			mc.GetMesh().draw();

			m_SceneData.m_SkinnedShader->Reset();
		}

		m_SceneData.m_SkinnedShader->Unuse();

		m_SceneData.m_TerrainShader->Use();

		m_SceneData.m_TerrainShader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_TerrainShader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));

		m_SceneData.m_TerrainShader->SetUniform("camera_position", &camera.GetPosition(), sizeof(glm::vec3));
		m_SceneData.m_TerrainShader->SetUniform("usePointLight", &m_SceneData.nPts, sizeof(int));
		m_SceneData.m_TerrainShader->SetUniform("useDirLight", &m_SceneData.nDirs, sizeof(int));
		m_SceneData.m_TerrainShader->SetUniform("pointLights", m_SceneData.m_PointsBuffer.data(), sizeof(PointLight) * 4);
		m_SceneData.m_TerrainShader->SetUniform("dirLights", m_SceneData.m_DirectionalsBuffer.data(), sizeof(DirectionalLight) * 4);

		if (!m_SceneData.m_TerrainShader->UpdateGlobalState()) {
			m_SceneData.m_TerrainShader->Unuse();
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, TerrainComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& tc = entity.GetComponent<TerrainComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			if (!mr.m_Rendered) continue;

			auto mesh = tc.GetMesh();
			if (!mesh || !mesh->IsMeshGenerated()) continue;

			glm::mat4 transform = tr.GetGlobalTransform();

			m_SceneData.m_TerrainShader->SetUniform("model", &transform, sizeof(glm::mat4));
			m_SceneData.m_TerrainShader->SetUniform("heightMult", &tc.heightMult, sizeof(float));
			m_SceneData.m_TerrainShader->SetUniform("uTextureScale", &tc.textureScale, sizeof(int));

			Material& mat = entity.GetComponent<MaterialComponent>().GetMaterial();

			m_SceneData.m_TerrainShader->SetUniform("albedo", &mat.GetAlbedo(), sizeof(glm::vec4));
			float rough = mat.GetRoughness();
			float metal = mat.GetMetallic();
			float ao = mat.GetAO();
			m_SceneData.m_TerrainShader->SetUniform("roughness", &rough, sizeof(float));
			m_SceneData.m_TerrainShader->SetUniform("metallic", &metal, sizeof(float));
			m_SceneData.m_TerrainShader->SetUniform("ao", &ao, sizeof(float));

			int hasA = mat.HasTexture(TextureType::Albedo) ? 1 : 0;
			int hasN = mat.HasTexture(TextureType::Normal) ? 1 : 0;
			int hasR = mat.HasTexture(TextureType::Roughness) ? 1 : 0;
			int hasM = mat.HasTexture(TextureType::Metallic) ? 1 : 0;
			int hasO = mat.HasTexture(TextureType::AO) ? 1 : 0;

			m_SceneData.m_TerrainShader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
			m_SceneData.m_TerrainShader->SetUniform("has_normal_texture", &hasN, sizeof(int));
			m_SceneData.m_TerrainShader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
			m_SceneData.m_TerrainShader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
			m_SceneData.m_TerrainShader->SetUniform("has_ao_texture", &hasO, sizeof(int));

			if (AssetManager::Instance().isAssetLoaded(tc.GetHeightMapId()))
			{
				m_SceneData.m_TerrainShader->SetTexture(
					"heightMap",
					AssetManager::Instance().getAsset<Texture2D>(tc.GetHeightMapId()).get()
				);
			}
			else {
				AssetToLoad tcAsset{};
				tcAsset.id = tc.GetHeightMapId();
				tcAsset.path = tc.GetHeightMapPath();
				tcAsset.type = AssetType::TEXTURE;

				AssetManager::Instance().loadAsset(tcAsset);
			}

			m_SceneData.m_TerrainShader->SetTexture("albedo_texture", mat.GetTexture(TextureType::Albedo));
			m_SceneData.m_TerrainShader->SetTexture("normal_texture", mat.GetTexture(TextureType::Normal));
			m_SceneData.m_TerrainShader->SetTexture("roughness_texture", mat.GetTexture(TextureType::Roughness));
			m_SceneData.m_TerrainShader->SetTexture("metallic_texture", mat.GetTexture(TextureType::Metallic));
			m_SceneData.m_TerrainShader->SetTexture("ao_texture", mat.GetTexture(TextureType::AO));

			if (!m_SceneData.m_TerrainShader->UpdateObject(&mat)) {
				continue;
			}

			mesh->draw();

			m_SceneData.m_TerrainShader->Reset();
		}

		m_SceneData.m_TerrainShader->Unuse();

		//std::cout << entityDraw << "/" << totalEntity << std::endl;

		{
			Renderer2D::Instance().BeginScene(camera);

			for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>())
			{
				Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

				auto& tr = entity.GetComponent<TransformComponent>();
				auto& sc = entity.GetComponent<SpriteComponent>();
				const auto& spec = sc.GetSpecification();
				if (!spec.Visible) continue;

				Texture* tex = sc.GetTexture();
				glm::mat4 T = tr.GetGlobalTransform();
				glm::vec4 uv = sc.GetEffectiveUV();

				Renderer2D::Instance().DrawQuad(
					T, tex, spec.Color, uv, spec.Tiling, spec.Offset, spec.SortingOrder
				);
			}

			Renderer2D::Instance().EndScene();
		}
	}

	void Renderer::RenderDebug(BaseCamera& camera)
	{
		/*if (PhysicEngine::Instance().GetDebugVertexArray())
		{
			glm::mat4 viewMatrix = camera.getViewMatrix();
			glm::mat4 projectionMatrix = camera.getProjectionMatrix();

			m_SceneData.m_PhysicDebugShader->Use();

			glm::mat4 model = glm::mat4(1.0f);

			m_SceneData.m_PhysicDebugShader->SetUniform("view", glm::value_ptr(viewMatrix), sizeof(glm::mat4));
			m_SceneData.m_PhysicDebugShader->SetUniform("projection", glm::value_ptr(projectionMatrix), sizeof(glm::mat4));
			m_SceneData.m_PhysicDebugShader->SetUniform("model", glm::value_ptr(model), sizeof(glm::mat4));

			m_SceneData.m_PhysicDebugShader->UpdateGlobalState();
			m_SceneData.m_PhysicDebugShader->UpdateObject(nullptr);

			PhysicEngine::Instance().GetDebugVertexArray()->Bind();

			RenderCommand::Instance().DrawArrays(DrawMode::LINES, static_cast<uint32_t>(PhysicEngine::Instance().GetDebugVertexArray()->GetVertexBuffers()[0]->GetSize() / sizeof(float) / 6));

			m_SceneData.m_PhysicDebugShader->Unuse();
		}*/
	}

	void Renderer::RenderSkybox(BaseCamera& camera)
	{
		/*m_SceneData.m_Skybox->Bind();

		glm::mat4 viewMatrix = camera.getViewMatrix();
		glm::mat4 projectionMatrix = camera.getProjectionMatrix();

		m_SceneData.m_Skybox->GetShader()->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_Skybox->GetShader()->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));

		m_SceneData.m_Skybox->GetShader()->UpdateGlobalState();

		m_SceneData.m_Skybox->GetShader()->SetTexture("skybox", m_SceneData.m_Skybox->GetMaterial()->GetTexture(TextureType::Albedo), Shader::SamplerType::SamplerCube);

		m_SceneData.m_Skybox->GetShader()->UpdateObject(m_SceneData.m_Skybox->GetMaterial());

		m_SceneData.m_Skybox->Draw();

		m_SceneData.m_Skybox->Unbind();*/

		const glm::mat4 V = camera.getViewMatrix();
		const glm::mat4 P = camera.getProjectionMatrix();

		m_SceneData.m_SkyboxHDR->Draw(V, P);
	}

	void Renderer::RenderUI(BaseCamera& camera, int fbW, int fbH, float dpi)
	{
		UIFBInfo fb{ fbW, fbH, dpi };
		m_SceneData.m_UI->Render(camera, fb);
	}

	void Renderer::EndScene()
	{

	}

	void Renderer::CollectLights(Scene& scene)
	{
		m_SceneData.nPts = 0;
		m_SceneData.nDirs = 0;

		for (auto e : scene.GetAllEntitiesWith<TransformComponent, LightComponent>())
		{
			Entity ent{ e, scene.GetRegistry() };
			const auto& tr = ent.GetComponent<TransformComponent>();
			const auto& lc = ent.GetComponent<LightComponent>();

			if (lc.lightType == LightComponent::LightType::DIRECTIONAL)
			{
				if (m_SceneData.nDirs < 4) {
					DirectionalLight L{};
					glm::vec3 dir = glm::vec3(0.0f, -1.0f, 0.0f);
					if (glm::length2(tr.Rotation) > 0.0f) {
						const glm::vec3 eulDeg = tr.Rotation;
						const glm::quat q = glm::quat(eulDeg);
						dir = glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f));
					}
					L.direction = dir;
					L.color = lc.directional_light.color;
					L.power = lc.directional_light.power;
					m_SceneData.m_DirectionalsBuffer[m_SceneData.nDirs++] = L;
				}
			}
			else if (lc.lightType == LightComponent::LightType::POINT)
			{
				if (m_SceneData.nPts < 4) {
					PointLight L{};
					L.position = glm::vec3(tr.Position);
					L.color = lc.point_light.color;
					L.attenuation = lc.point_light.attenuation;
					L.power = lc.point_light.power;
					m_SceneData.m_PointsBuffer[m_SceneData.nPts++] = L;
				}
			}
		}

		for (int i = m_SceneData.nDirs; i < 4; ++i) m_SceneData.m_DirectionalsBuffer[i] = DirectionalLight{};
		for (int i = m_SceneData.nPts; i < 4; ++i) m_SceneData.m_PointsBuffer[i] = PointLight{};
	}

	Scene* Renderer::GetScene()
	{
		return m_SceneData.m_Scene;
	}

	double Renderer::GetTime()
	{
		return glfwGetTime();
	}
}
