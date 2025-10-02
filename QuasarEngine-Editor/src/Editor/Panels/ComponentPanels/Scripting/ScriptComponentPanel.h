#pragma once

#include <string>

#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
	class Entity;

	class ScriptComponentPanel
	{
	public:
		ScriptComponentPanel(const std::string& projectPath);
		~ScriptComponentPanel() = default;

		void Render(Entity entity);

	private:
		std::string m_ProjectPath;
		char m_LocalBuffer[256];

		UUID m_LastEntityID;
		std::string m_LastFullPath;
	};
}
