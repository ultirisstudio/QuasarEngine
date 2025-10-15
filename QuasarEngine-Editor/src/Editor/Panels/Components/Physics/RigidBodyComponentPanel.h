#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class RigidBodyComponentPanel : public IComponentPanel
	{
	public:
		RigidBodyComponentPanel() = default;
		~RigidBodyComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}