#pragma once

namespace QuasarEngine
{
	class Entity;

	class ScriptComponentPanel
	{
	public:
		ScriptComponentPanel() = default;
		~ScriptComponentPanel() = default;

		void Render(Entity entity);
	};
}