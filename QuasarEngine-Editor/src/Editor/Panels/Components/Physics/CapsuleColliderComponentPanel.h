#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class CapsuleColliderComponentPanel : public IComponentPanel
	{
	public:
		CapsuleColliderComponentPanel() = default;
		~CapsuleColliderComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}