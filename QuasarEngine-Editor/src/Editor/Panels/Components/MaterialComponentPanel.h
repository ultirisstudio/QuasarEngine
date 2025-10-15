#pragma once

#include <Editor/Panels/IComponentPanel.h>

#include <memory>
#include <string>

namespace QuasarEngine
{
	class Entity;
	class Material;

	struct MaterialSpecification;

	class MaterialComponentPanel : public IComponentPanel
	{
	public:
		MaterialComponentPanel();
		~MaterialComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
