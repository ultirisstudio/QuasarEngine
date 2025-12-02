#pragma once

#include <QuasarEngine/Renderer/IRenderTechnique.h>

namespace QuasarEngine
{
	static constexpr int QE_MAX_BONES = 100;

	class PBRSkinTechnique : public IRenderTechnique
	{
	public:
		PBRSkinTechnique(SkyboxHDR* skybox);
		~PBRSkinTechnique();

		void Begin(RenderContext& ctx) override;
		void Submit(RenderContext& ctx, RenderObject& obj) override;
		void End() override;

	private:
		std::shared_ptr<Shader> m_Shader;
		SkyboxHDR* m_Skybox;
		std::array<glm::mat4, QE_MAX_BONES> m_IdentityBones;
	};
}