#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
	class Entity;

	class TerrainComponentPanel
	{
	public:
		TerrainComponentPanel();
		~TerrainComponentPanel() = default;

		void Update();

		void Render(Entity entity);
	};
}
