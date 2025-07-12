#pragma once

namespace QuasarEngine
{
	class Entity;

	class LightComponentPanel
	{
	public:
		LightComponentPanel() = default;
		~LightComponentPanel() = default;

		void Render(Entity entity);
	};
}
