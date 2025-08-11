#pragma once

#include <string>

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

		uint64_t m_LastEntityID = 0;
		std::string m_LastFullPath;
	};
}
