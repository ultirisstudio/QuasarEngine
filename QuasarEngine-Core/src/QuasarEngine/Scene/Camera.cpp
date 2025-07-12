#include "qepch.h"
#include <QuasarEngine/Scene/Camera.h>

namespace QuasarEngine
{
	Camera::Camera() : m_projectionMatrix(1.0f), m_fov(45.0f), m_minFov(15.0f), m_maxFov(95.0f), m_ViewportSize(1.0f, 1.0f), m_TransformComponent(nullptr)
	{

	}

	void Camera::Init(TransformComponent* transformComponent)
	{
		m_TransformComponent = transformComponent;
	}

	const glm::mat4 Camera::getViewMatrix() const
	{
		return m_TransformComponent->GetGlobalViewMatrix();
	}

	const glm::mat4& Camera::getProjectionMatrix() const
	{
		return m_projectionMatrix;
	}

	void Camera::updateProjectionMatrix()
	{
		switch (m_cameraType)
		{
		case CameraType::PERSPECTIVE:
		{
			m_projectionMatrix = glm::perspective(glm::radians(GetFov()), m_ViewportSize.x / m_ViewportSize.y, 0.1f, 1000.0f);
			break;
		}
		case CameraType::ORTHOGRAPHIC:
		{
			float aspect_ratio = m_ViewportSize.x / m_ViewportSize.y;

			if (aspect_ratio > 1.7778) {
				float half_height = m_ViewportSize.y / 1080.0f / 2.0f;
				m_projectionMatrix = glm::ortho(-half_height * aspect_ratio, half_height * aspect_ratio, -half_height, half_height, 0.1f, 100.0f);
			}
			else {
				float half_width = m_ViewportSize.x / 1920.0f / 2.0f;
				m_projectionMatrix = glm::ortho(-half_width, half_width, -half_width / aspect_ratio, half_width / aspect_ratio, 0.1f, 100.0f);
			}
			break;
		}
		default:
			break;
		}
	}

	glm::mat4 Camera::GetTransform()
	{
		return m_TransformComponent->GetGlobalTransform();
	}

	glm::vec3 Camera::GetFront()
	{
		glm::vec3 front = m_TransformComponent->Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
		return glm::normalize(front);
	}

	float Camera::GetFov() const
	{
		return m_fov;
	}

	void Camera::SetFov(float fov)
	{
		m_fov = fov;
		
		updateProjectionMatrix();
	}

	void Camera::OnResize(float width, float height)
	{
		m_ViewportSize.x = width;
		m_ViewportSize.y = height;
		
		updateProjectionMatrix();
	}
}