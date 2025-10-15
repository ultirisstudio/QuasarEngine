#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class MeshComponentPanel : public IComponentPanel
	{
	public:
		MeshComponentPanel() = default;
		~MeshComponentPanel() = default;
		
		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
