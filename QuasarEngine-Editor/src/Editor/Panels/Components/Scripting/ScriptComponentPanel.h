#pragma once

#include <string>

#include <QuasarEngine/Core/UUID.h>

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class ScriptComponentPanel : public IComponentPanel
	{
	public:
		ScriptComponentPanel(const std::string& projectPath);
		~ScriptComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }

	private:
		std::string m_ProjectPath;
		char m_LocalBuffer[256];

		UUID m_LastEntityID;
		std::string m_LastFullPath;
	};
}
