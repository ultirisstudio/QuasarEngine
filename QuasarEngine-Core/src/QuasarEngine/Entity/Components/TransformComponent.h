#pragma once

#include <QuasarEngine/Entity/Component.h>

#include <glm/glm.hpp>

namespace QuasarEngine
{
	class TransformComponent : public Component
	{
	private:
		glm::mat4 CalculateViewMatrix(glm::mat4 transform) const;
	public:
		glm::vec3 Position = { 0.0f,0.0f,0.0f };
		glm::vec3 Rotation = { 0.0f,0.0f,0.0f };
		glm::vec3 Scale = { 1.0f,1.0f,1.0f };

		TransformComponent() = default;
		TransformComponent(const glm::vec3& position);

		void SetPosition(const glm::vec3& position) { Position = position; }
		void SetRotation(const glm::vec3& rotation) { Rotation = rotation; }
		void SetScale(const glm::vec3& scale) { Scale = scale; }

		glm::mat4 GetGlobalTransform() const;
		glm::mat4 GetLocalTransform() const;

		glm::mat4 GetLocalViewMatrix() const;
		glm::mat4 GetGlobalViewMatrix() const;
	};
}