#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <queue>

#include "SceneObject.h"

namespace QuasarEngine
{
	class SceneManager
	{
	public:
		struct ModelToLoad
		{
			std::string path;
		};

		enum class PendingAction
		{
			None,
			LoadScene,
			LoadSceneWithPath,
			ReloadScene,
			CreateNewScene
		};

	public:
		SceneManager(std::filesystem::path assetPath);
		~SceneManager();

		void Update(double dt);

		Scene& GetActiveScene() { return m_SceneObject->GetScene(); }
		const Scene& GetActiveScene() const { return m_SceneObject->GetScene(); }

		SceneObject& GetSceneObject() { return *m_SceneObject; }

		void LoadModel(const ModelToLoad& modelToLoad);

		void AddGameObject(const std::string& file);

		void AddCube();
		void AddSphere();
		void AddUVSphere();
		void AddPlane();

		void SaveScene();
		void LoadScene();
		void LoadScene(std::string filePath);

		void ReloadScene(std::string filePath);

		void createNewScene();

		void OpenExternalFile();
	private:
		std::unique_ptr<SceneObject> m_SceneObject;

		std::queue<ModelToLoad> m_ModelsToLoad;

		std::filesystem::path m_AssetPath;

		PendingAction m_PendingAction = PendingAction::None;
		std::string m_PendingActionFile;
	};
}

