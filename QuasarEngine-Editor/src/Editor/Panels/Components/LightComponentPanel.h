#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class LightComponentPanel : public IComponentPanel
	{
	public:
		LightComponentPanel() = default;
		~LightComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
