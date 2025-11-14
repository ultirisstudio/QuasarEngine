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

		std::string m_Name;
		std::string m_Path;
	public:
		SceneObject();

		void CreateScene();

		Scene& GetScene() { return *m_Scene; }

		const std::string& GetPath() const { return m_Path; }
		const std::string& GetName() const { return m_Name; }

		char* GetNameData() { return m_Name.data(); }

		void SetName(std::string name) { m_Name = name; }
		void SetPath(std::string path) { m_Path = path; }
	};
}