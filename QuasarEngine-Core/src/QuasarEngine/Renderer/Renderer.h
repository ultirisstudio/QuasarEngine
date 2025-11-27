#pragma once

//#include <QuasarEngine/Resources/BasicSkybox.h>
#include <QuasarEngine/Resources/SkyboxHDR.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Scene/BaseCamera.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Scripting/ScriptSystem.h>

#include <QuasarEngine/Resources/Lights/PointLight.h>
#include <QuasarEngine/Resources/Lights/DirectionalLight.h>

#include <QuasarEngine/UI/UISystem.h>

#include <QuasarEngine/Core/Singleton.h>

namespace QuasarEngine
{
	static constexpr int QE_MAX_BONES = 100;

	class Renderer : public Singleton<Renderer>
	{
	public:
		struct SceneData
		{
			Scene* m_Scene;

			std::shared_ptr<Shader> m_Shader;
			//std::shared_ptr<Shader> m_PhysicDebugShader;
			std::shared_ptr<Shader> m_TerrainShader;
			std::shared_ptr<Shader> m_SkinnedShader;
			std::shared_ptr<Shader> m_PointCloudShader;

			std::array<glm::mat4, QE_MAX_BONES> m_IdentityBones;

			//std::shared_ptr<BasicSkybox> m_Skybox;
			std::shared_ptr<SkyboxHDR> m_SkyboxHDR;

			std::unique_ptr<ScriptSystem> m_ScriptSystem;

			std::unique_ptr<UISystem> m_UI;

			std::array<PointLight, 4> m_PointsBuffer;
			std::array<DirectionalLight, 4> m_DirectionalsBuffer;
			int nPts = 0, nDirs = 0;

			std::array<std::shared_ptr<Framebuffer>, 4> m_DirShadowFBO;
			std::array<glm::mat4, 4> m_DirLightVP;
			std::shared_ptr<Shader> m_ShadowDir_Static;

			struct {
				int   mapSize = 2048;
				float bias = 0.0025f;
				int   pcfRadius = 1;
				float orthoRange = 50.f;
				float nearPlane = 0.1f;
				float farPlane = 150.f;
			} m_DirShadow;
		};
		SceneData m_SceneData;

		void Initialize();
		void Shutdown();

		void BeginScene(Scene& scene);
		void Render(BaseCamera& camera);
		void RenderDebug(BaseCamera& camera);
		void RenderSkybox(BaseCamera& camera);
		void RenderUI(BaseCamera& camera, int fbW, int fbH, float dpi);
		void EndScene();

		void BuildLight(BaseCamera& camera);

		Scene* GetScene();

		double GetTime();
	};
}