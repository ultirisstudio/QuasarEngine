#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine
{
	class BaseCamera
	{
	public:
		virtual const glm::mat4 getViewMatrix() const = 0;
		virtual const glm::mat4& getProjectionMatrix() const = 0;

		virtual glm::mat4 GetTransform() = 0;

		virtual glm::vec3 GetFront() = 0;
	};
}