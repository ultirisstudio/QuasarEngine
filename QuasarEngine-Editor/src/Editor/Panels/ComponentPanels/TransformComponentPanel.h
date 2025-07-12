#pragma once

#include <glm/glm.hpp>
#include <string>

namespace QuasarEngine
{
	class Entity;

	class TransformComponentPanel
	{
	public:
		TransformComponentPanel() = default;
		~TransformComponentPanel() = default;

		void Render(Entity entity);
	private:
		void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
	};
}