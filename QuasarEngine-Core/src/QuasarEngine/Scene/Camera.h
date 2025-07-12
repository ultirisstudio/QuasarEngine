#pragma once

#include "BaseCamera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <QuasarEngine/Entity/Components/TransformComponent.h>

namespace QuasarEngine
{
	enum class CameraType
	{
		PERSPECTIVE = 0,
		ORTHOGRAPHIC
	};

	class Camera : public BaseCamera
	{
	private:
		glm::mat4 m_projectionMatrix;

		float m_minFov;
		float m_maxFov;
		float m_fov;

		CameraType m_cameraType = CameraType::PERSPECTIVE;

		glm::vec2 m_ViewportSize;

		TransformComponent* m_TransformComponent;

		friend class EntityPropertiePanel;
		friend class CameraComponent;
		friend class Scene;
	public:
		Camera();
		void Init(TransformComponent* transformComponent);

		const glm::mat4 getViewMatrix() const override;
		const glm::mat4& getProjectionMatrix() const override;

		void updateProjectionMatrix();

		glm::mat4 GetTransform() override;

		glm::vec3 GetFront() override;

		float GetFov() const;
		void SetFov(float fov);

		void OnResize(float width, float height);
	};
}