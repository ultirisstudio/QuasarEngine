#pragma once

#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Materials/CubeMap.h>
#include <QuasarEngine/Shader/Shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace QuasarEngine
{
	class Skybox
	{
	private:
		std::shared_ptr<Model> m_Model;

		//std::shared_ptr<Shader> m_EquirectangularToCubemapShader;
		//std::shared_ptr<Shader> m_IrradianceShader;
		//std::shared_ptr<Shader> m_BackgroundShader;
		//std::shared_ptr<Shader> m_PrefilterShader;
		//std::shared_ptr<Shader> m_BrdfShader;

		unsigned int hdrTexture;
		unsigned int envCubemap;
		unsigned int irradianceMap;
		unsigned int brdfLUTTexture;
		unsigned int prefilterMap;

		unsigned int quadVAO = 0;
		unsigned int quadVBO;

		void RenderQuad();
	public:
		Skybox();

		//void BindCubeMap();

		void BindIrradianceMap(int index = 0);

		void BindPrefilterMap(int index = 0);

		void BindBrdfLUT(int index = 0);

		void DrawSkybox(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

		/*std::shared_ptr<Shader> GetShader()
		{
			return m_BackgroundShader;
		}

		Model* GetModel()
		{
			return m_Model.get();
		}*/
	};
}

