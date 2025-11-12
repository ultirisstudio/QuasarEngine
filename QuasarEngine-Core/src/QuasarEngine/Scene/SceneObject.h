#pragma once

#include <filesystem>

#include <QuasarEngine/Scene/Scene.h>

namespace QuasarEngine
{
	class Entity;
	class Camera;

	class SceneObject
	{
	private:
		std::unique_ptr<Scene> m_Scene;
		std::unique_ptr<Camera> m_Camera;

		std::string m_Name;
		std::string m_Path;
	public:
		SceneObject();

		void CreateScene();

		Scene& GetScene() { return *m_Scene; }

		std::string GetPath() { return m_Path; }
		inline std::string GetName() { return m_Name; }
		inline char* GetNameData() { return m_Name.data(); }
		void SetName(std::string name) { m_Name = name; }
		void SetPath(std::string path) { m_Path = path; }
	};
}