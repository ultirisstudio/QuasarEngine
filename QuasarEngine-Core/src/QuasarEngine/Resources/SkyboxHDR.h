#pragma once

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Resources/Materials/CubeMap.h>
#include <QuasarEngine/Shader/Shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace QuasarEngine
{
	class SkyboxHDR
	{
	private:
		std::shared_ptr<Mesh> m_CubeMesh;
		std::shared_ptr<Mesh> m_QuadMesh;

		std::shared_ptr<Shader> m_EquirectangularToCubemapShader;
		std::shared_ptr<Shader> m_IrradianceShader;
		std::shared_ptr<Shader> m_BackgroundShader;
		std::shared_ptr<Shader> m_PrefilterShader;
		std::shared_ptr<Shader> m_BrdfShader;

		unsigned int hdrTexture;
		unsigned int envCubemap;
		unsigned int irradianceMap;
		unsigned int brdfLUTTexture;
		unsigned int prefilterMap;

		unsigned int quadVAO = 0;
		unsigned int quadVBO;

		void RenderQuad();
	public:
		SkyboxHDR();

		void BindCubeMap();

		void BindIrradianceMap();

		void BindPrefilterMap();

		void BindBrdfLUT();

		void Draw(const glm::mat4& view, const glm::mat4& projection);
	};
}
