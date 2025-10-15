#pragma once

#include <glm/glm.hpp>
#include <string>

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class TransformComponentPanel : public IComponentPanel
	{
	public:
		TransformComponentPanel() = default;
		~TransformComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }

	private:
		void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
	};
}