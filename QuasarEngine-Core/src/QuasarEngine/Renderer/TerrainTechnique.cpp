#include "qepch.h"

#include <QuasarEngine/Renderer/TerrainTechnique.h>

#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Entity/Components/TerrainComponent.h>

namespace QuasarEngine
{
	TerrainTechnique::TerrainTechnique(SkyboxHDR* skybox)
		: m_Skybox(skybox)
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

		desc.cullMode = Shader::CullMode::Back;

		const std::string tName = "gpuheight";

		std::string tVertPath = basePath + tName + extFor(api, Shader::ShaderStageType::Vertex);
		std::string tTcsPath = basePath + tName + extFor(api, Shader::ShaderStageType::TessControl);
		std::string tTesPath = basePath + tName + extFor(api, Shader::ShaderStageType::TessEval);
		std::string tFragPath = basePath + tName + extFor(api, Shader::ShaderStageType::Fragment);

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				tVertPath,
				"",
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal", true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::TessControl,
				tTcsPath,
				"",
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

		desc.globalUniforms = {
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

		desc.objectUniforms = {
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

		desc.samplers = {
			{"heightMap",        1, 1, Shader::StageToBit(Shader::ShaderStageType::TessEval)},

			{"albedo_texture",   1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture",   1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture",1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture",       1, 6, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
		};

		desc.blendMode = Shader::BlendMode::None;
		desc.cullMode = Shader::CullMode::Back;
		desc.fillMode = Shader::FillMode::Solid;
		desc.depthFunc = Shader::DepthFunc::Less;
		desc.depthTestEnable = true;
		desc.depthWriteEnable = true;

		desc.topology = Shader::PrimitiveTopology::PatchList;

		desc.patchControlPoints = 4;

		desc.enableDynamicViewport = true;
		desc.enableDynamicScissor = true;

		m_Shader = Shader::Create(desc);
	}

	TerrainTechnique::~TerrainTechnique()
	{
		m_Shader.reset();
	}

	void TerrainTechnique::Begin(RenderContext& ctx)
	{
		m_Shader->Use();

		m_Shader->SetUniform("view", &ctx.view , sizeof(glm::mat4));
		m_Shader->SetUniform("projection", &ctx.projection, sizeof(glm::mat4));

		m_Shader->SetUniform("camera_position", &ctx.cameraPosition, sizeof(glm::vec3));
		m_Shader->SetUniform("usePointLight", &ctx.numPointLights, sizeof(int));
		m_Shader->SetUniform("useDirLight", &ctx.numDirLights, sizeof(int));
		m_Shader->SetUniform("pointLights", ctx.pointLights, sizeof(PointLight) * 4);
		m_Shader->SetUniform("dirLights", ctx.dirLights, sizeof(DirectionalLight) * 4);

		m_Shader->UpdateGlobalState();
	}

	void TerrainTechnique::Submit(RenderContext& ctx, RenderObject& obj)
	{
		if (HasFlag(obj.flags, RenderFlags::Skinned))   return;
		if (HasFlag(obj.flags, RenderFlags::PointCloud)) return;

		auto& tc = obj.entity.GetComponent<TerrainComponent>();

		m_Shader->SetUniform("model", &obj.model, sizeof(glm::mat4));
		m_Shader->SetUniform("heightMult", &tc.heightMult, sizeof(float));
		m_Shader->SetUniform("uTextureScale", &tc.textureScale, sizeof(int));

		Material& material = *obj.material;

		m_Shader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
		float rough = material.GetRoughness();
		float metal = material.GetMetallic();
		float ao = material.GetAO();
		m_Shader->SetUniform("roughness", &rough, sizeof(float));
		m_Shader->SetUniform("metallic", &metal, sizeof(float));
		m_Shader->SetUniform("ao", &ao, sizeof(float));

		int hasA = material.HasTexture(TextureType::Albedo) ? 1 : 0;
		int hasN = material.HasTexture(TextureType::Normal) ? 1 : 0;
		int hasR = material.HasTexture(TextureType::Roughness) ? 1 : 0;
		int hasM = material.HasTexture(TextureType::Metallic) ? 1 : 0;
		int hasO = material.HasTexture(TextureType::AO) ? 1 : 0;

		m_Shader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
		m_Shader->SetUniform("has_normal_texture", &hasN, sizeof(int));
		m_Shader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
		m_Shader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
		m_Shader->SetUniform("has_ao_texture", &hasO, sizeof(int));

		if (AssetManager::Instance().isAssetLoaded(tc.GetHeightMapId()))
		{
			m_Shader->SetTexture(
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

		m_Shader->SetTexture("albedo_texture", material.GetTexture(TextureType::Albedo));
		m_Shader->SetTexture("normal_texture", material.GetTexture(TextureType::Normal));
		m_Shader->SetTexture("roughness_texture", material.GetTexture(TextureType::Roughness));
		m_Shader->SetTexture("metallic_texture", material.GetTexture(TextureType::Metallic));
		m_Shader->SetTexture("ao_texture", material.GetTexture(TextureType::AO));

		m_Shader->UpdateObject(&material);

		//if (obj.instanceCount > 1)
			//obj.mesh->drawInstanced(obj.instanceCount);
		//else
		obj.mesh->draw();

		m_Shader->Reset();
	}

	void TerrainTechnique::End()
	{
		m_Shader->Unuse();
	}
}