#pragma once

#include <QuasarEngine/Renderer/IRenderTechnique.h>

namespace QuasarEngine
{
	class TerrainTechnique : public IRenderTechnique
	{
	public:
		TerrainTechnique(SkyboxHDR* skybox);
		~TerrainTechnique();

		void Begin(RenderContext& ctx) override;
		void Submit(RenderContext& ctx, RenderObject& obj) override;
		void End() override;

	private:
		std::shared_ptr<Shader> m_Shader;
		SkyboxHDR* m_Skybox;
	};
}