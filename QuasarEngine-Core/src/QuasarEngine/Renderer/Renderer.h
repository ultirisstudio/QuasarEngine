#pragma once

#include <QuasarEngine/Resources/ResourceManager.h>
#include <QuasarEngine/Resources/BasicSkybox.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Scene/BaseCamera.h>
#include <QuasarEngine/Asset/AssetRegistry.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Scripting/ScriptSystem.h>

#include <QuasarEngine/Renderer/RenderCommand.h>

#include <filesystem>

namespace QuasarEngine
{
	class Renderer
	{
	public:		
		struct SceneData
		{
			Scene* m_Scene;
			std::unique_ptr<AssetManager> m_AssetManager;

			std::shared_ptr<Shader> m_Shader;

			std::shared_ptr<BasicSkybox> m_Skybox;

			std::unique_ptr<ScriptSystem> m_ScriptSystem;
		};
		static SceneData m_SceneData;

		static void Init();
		static void Shutdown();

		static void BeginScene(Scene& scene);
		static void Render(BaseCamera& camera);
		static void RenderSkybox(BaseCamera& camera);
		static void EndScene();

		static Scene* GetScene();

		static double GetTime();

		static RendererAPI::API GetAPI();
	};
}