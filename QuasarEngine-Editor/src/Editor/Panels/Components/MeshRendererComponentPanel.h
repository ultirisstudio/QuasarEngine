#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class MeshRendererComponentPanel : public IComponentPanel
	{
	public:
		MeshRendererComponentPanel() = default;
		~MeshRendererComponentPanel() = default;
		
		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
