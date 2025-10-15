#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class SphereColliderComponentPanel : public IComponentPanel
	{
	public:
		SphereColliderComponentPanel() = default;
		~SphereColliderComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}