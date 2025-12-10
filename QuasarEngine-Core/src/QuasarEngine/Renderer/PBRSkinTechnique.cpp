#include "qepch.h"

#include <QuasarEngine/Renderer/PBRSkinTechnique.h>

#include <QuasarEngine/Entity/Components/Animation/AnimationComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>

namespace QuasarEngine
{
	PBRSkinTechnique::PBRSkinTechnique(SkyboxHDR* skybox)
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

		const std::string name = "basic_anim";

		std::string vertPath = basePath + name + extFor(api, Shader::ShaderStageType::Vertex);
		std::string fragPath = basePath + name + extFor(api, Shader::ShaderStageType::Fragment);

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				vertPath,
				"",
				{
					{0, Shader::ShaderIOType::Vec3,  "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3,  "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2,  "inTexCoord", true, ""},
					{3, Shader::ShaderIOType::Vec3,  "inTangent",  true, ""},
					{4, Shader::ShaderIOType::Vec4,  "inColor",    true, ""},
					{5, Shader::ShaderIOType::IVec4, "inBoneIds",  true, ""},
					{6, Shader::ShaderIOType::Vec4,  "inWeights",  true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				fragPath,
				"",
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
			{"view",			Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),				offsetof(GlobalUniforms, view), 0, 0, globalUniformsFlags},
			{"projection",		Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),				offsetof(GlobalUniforms, projection), 0, 0, globalUniformsFlags},
			{"camera_position", Shader::ShaderUniformType::Vec3,	sizeof(glm::vec3),				offsetof(GlobalUniforms, camera_position), 0, 0, globalUniformsFlags},

			{"usePointLight",	Shader::ShaderUniformType::Int,		sizeof(int),					offsetof(GlobalUniforms, usePointLight), 0, 0, globalUniformsFlags},
			{"useDirLight",		Shader::ShaderUniformType::Int,		sizeof(int),					offsetof(GlobalUniforms, useDirLight), 0, 0, globalUniformsFlags},

			{"prefilterLevels",	Shader::ShaderUniformType::Int,		sizeof(int),					offsetof(GlobalUniforms, prefilterLevels), 0, 0, globalUniformsFlags},

			{"pointLights",		Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4,			offsetof(GlobalUniforms, pointLights), 0, 0, globalUniformsFlags},
			{"dirLights",		Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4,	offsetof(GlobalUniforms, dirLights), 0, 0, globalUniformsFlags}
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

			glm::mat4 finalBonesMatrices[QE_MAX_BONES];
		};

		static_assert(sizeof(ObjectUniforms) % 16 == 0, "ObjectUniforms must be 16-aligned");


		static_assert(sizeof(ObjectUniforms) % 16 == 0, "ObjectUniforms must be 16-aligned");

		constexpr Shader::ShaderStageFlags objectUniformsFlags =
			Shader::StageToBit(Shader::ShaderStageType::Vertex) |
			Shader::StageToBit(Shader::ShaderStageType::Fragment);

		desc.objectUniforms = {
			{"model",					Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),  offsetof(ObjectUniforms, model),					1, 0, objectUniformsFlags},

			{"albedo",					Shader::ShaderUniformType::Vec4,	sizeof(glm::vec4),  offsetof(ObjectUniforms, albedo),					1, 0, objectUniformsFlags},

			{"roughness",				Shader::ShaderUniformType::Float,	sizeof(float),      offsetof(ObjectUniforms, roughness),				1, 0, objectUniformsFlags},
			{"metallic",				Shader::ShaderUniformType::Float,	sizeof(float),      offsetof(ObjectUniforms, metallic),					1, 0, objectUniformsFlags},
			{"ao",						Shader::ShaderUniformType::Float,	sizeof(float),      offsetof(ObjectUniforms, ao),						1, 0, objectUniformsFlags},

			{"has_albedo_texture",		Shader::ShaderUniformType::Int,		sizeof(int),		offsetof(ObjectUniforms, has_albedo_texture),		1, 0, objectUniformsFlags},
			{"has_normal_texture",		Shader::ShaderUniformType::Int,		sizeof(int),		offsetof(ObjectUniforms, has_normal_texture),		1, 0, objectUniformsFlags},
			{"has_roughness_texture",	Shader::ShaderUniformType::Int,		sizeof(int),		offsetof(ObjectUniforms, has_roughness_texture),	1, 0, objectUniformsFlags},
			{"has_metallic_texture",	Shader::ShaderUniformType::Int,		sizeof(int),		offsetof(ObjectUniforms, has_metallic_texture),		1, 0, objectUniformsFlags},
			{"has_ao_texture",			Shader::ShaderUniformType::Int,		sizeof(int),		offsetof(ObjectUniforms, has_ao_texture),			1, 0, objectUniformsFlags},

			{"finalBonesMatrices",		Shader::ShaderUniformType::Unknown,	sizeof(glm::mat4) * QE_MAX_BONES, offsetof(ObjectUniforms, finalBonesMatrices),  1, 0, objectUniformsFlags },
		};

		desc.samplers = {
			{"albedo_texture",   1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture",   1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture",1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture",       1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"irradiance_map",   1, 6, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"prefilter_map",    1, 7, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"brdf_lut",         1, 8, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
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

		m_Shader = Shader::Create(desc);

		for (int i = 0; i < QE_MAX_BONES; ++i)
			m_IdentityBones[i] = glm::mat4(1.0f);
	}

	PBRSkinTechnique::~PBRSkinTechnique()
	{
		m_Shader.reset();
	}

	void PBRSkinTechnique::Begin(RenderContext& ctx)
	{
		m_Shader->Use();

		m_Shader->SetUniform("view", &ctx.view, sizeof(glm::mat4));
		m_Shader->SetUniform("projection", &ctx.projection, sizeof(glm::mat4));
		m_Shader->SetUniform("camera_position",
			&ctx.cameraPosition, sizeof(glm::vec3));

		m_Shader->SetUniform("usePointLight", &ctx.numPointLights, sizeof(int));
		m_Shader->SetUniform("useDirLight", &ctx.numDirLights, sizeof(int));
		m_Shader->SetUniform("pointLights",
			ctx.pointLights, sizeof(PointLight) * 4);
		m_Shader->SetUniform("dirLights",
			ctx.dirLights, sizeof(DirectionalLight) * 4);

		if (m_Skybox)
		{
			int prefilterLevels = m_Skybox->GetSettings().prefilterMipLevels;
			m_Shader->SetUniform("prefilterLevels",
				&prefilterLevels, sizeof(int));

			m_Shader->SetTexture("irradiance_map",
				m_Skybox->GetIrradianceMap().get());
			m_Shader->SetTexture("prefilter_map",
				m_Skybox->GetPrefilterMap().get());
			m_Shader->SetTexture("brdf_lut",
				m_Skybox->GetBrdfLUT().get());
		}

		m_Shader->UpdateGlobalState();
	}

	void PBRSkinTechnique::Submit(RenderContext& ctx, RenderObject& obj)
	{
		if (HasFlag(obj.flags, RenderFlags::PointCloud)) return;
		if (HasFlag(obj.flags, RenderFlags::Terrain))    return;

		auto FindAnimatorForEntity = [&](Entity e) -> AnimationComponent*
			{
				Entity current = e;

				while (current.IsValid())
				{
					if (current.HasComponent<AnimationComponent>())
						return &current.GetComponent<AnimationComponent>();

					if (!current.HasComponent<HierarchyComponent>())
						break;

					const auto& h = current.GetComponent<HierarchyComponent>();
					if (h.m_Parent == UUID::Null())
						break;

					auto parentOpt = ctx.scene->GetEntityByUUID(h.m_Parent);
					if (!parentOpt.has_value())
						break;

					current = *parentOpt;
				}

				return nullptr;
			};


		Material& material = *obj.material;

		m_Shader->SetUniform("model", &obj.model, sizeof(glm::mat4));

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

		m_Shader->SetTexture("albedo_texture", material.GetTexture(TextureType::Albedo));
		m_Shader->SetTexture("normal_texture", material.GetTexture(TextureType::Normal));
		m_Shader->SetTexture("roughness_texture", material.GetTexture(TextureType::Roughness));
		m_Shader->SetTexture("metallic_texture", material.GetTexture(TextureType::Metallic));
		m_Shader->SetTexture("ao_texture", material.GetTexture(TextureType::AO));

		m_Shader->SetTexture("irradiance_map", m_Skybox->GetIrradianceMap().get());
		m_Shader->SetTexture("prefilter_map", m_Skybox->GetPrefilterMap().get());
		m_Shader->SetTexture("brdf_lut", m_Skybox->GetBrdfLUT().get());

		const AnimationComponent* anim = FindAnimatorForEntity(obj.entity);
		if (anim && !anim->GetFinalBoneMatrices().empty()) {
			std::vector<glm::mat4> mats = anim->GetFinalBoneMatrices();
			const size_t n = std::min(mats.size(), (size_t)QE_MAX_BONES);
			if (n == (size_t)QE_MAX_BONES) {
				m_Shader->SetUniform("finalBonesMatrices", mats.data(), sizeof(glm::mat4) * QE_MAX_BONES);
			}
			else {
				std::array<glm::mat4, QE_MAX_BONES> tmp = m_IdentityBones;
				std::memcpy(tmp.data(), mats.data(), sizeof(glm::mat4) * n);
				m_Shader->SetUniform("finalBonesMatrices", tmp.data(), sizeof(glm::mat4) * QE_MAX_BONES);
			}
		}
		else {
			m_Shader->SetUniform("finalBonesMatrices",
				m_IdentityBones.data(),
				sizeof(glm::mat4) * QE_MAX_BONES);
		}

		m_Shader->UpdateObject(&material);

		//if (obj.instanceCount > 1)
			//obj.mesh->drawInstanced(obj.instanceCount);
		//else
		obj.mesh->draw();

		m_Shader->Reset();
	}

	void PBRSkinTechnique::End()
	{
		m_Shader->Unuse();
	}
}