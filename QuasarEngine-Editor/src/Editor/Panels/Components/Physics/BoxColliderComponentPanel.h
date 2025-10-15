#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class BoxColliderComponentPanel : public IComponentPanel
	{
	public:
		BoxColliderComponentPanel() = default;
		~BoxColliderComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}