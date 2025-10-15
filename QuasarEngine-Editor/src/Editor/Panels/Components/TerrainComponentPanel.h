#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class TerrainComponentPanel : public IComponentPanel
	{
	public:
		TerrainComponentPanel();
		~TerrainComponentPanel() = default;

		void Update();

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
