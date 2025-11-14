#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine
{
	class BaseCamera
	{
	public:
		virtual ~BaseCamera() = default;

		virtual const glm::mat4& getViewMatrix() const = 0;
		virtual const glm::mat4& getProjectionMatrix() const = 0;

		virtual glm::vec3 GetPosition() const = 0;
		virtual glm::mat4 GetTransform() const = 0;
		virtual glm::vec3 GetFront() const = 0;
	};
}