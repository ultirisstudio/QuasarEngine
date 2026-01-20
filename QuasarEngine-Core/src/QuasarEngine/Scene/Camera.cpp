#include "qepch.h"
#include <QuasarEngine/Scene/Camera.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace QuasarEngine
{
	/*Camera::Camera() : m_projectionMatrix(1.0f), m_fov(60.0f), m_minFov(15.0f), m_maxFov(95.0f), m_nearZ(0.1f), m_farZ(100.0f), m_ViewportSize(1.0f, 1.0f), m_TransformComponent(nullptr), m_cameraType(CameraType::PERSPECTIVE)
	{

	}

	void Camera::Init(TransformComponent* transformComponent)
	{
		m_TransformComponent = transformComponent;
		updateProjectionMatrix();
	}

	const glm::mat4& Camera::getViewMatrix() const
	{
		return m_TransformComponent ? m_TransformComponent->GetGlobalViewMatrix() : glm::mat4(1.0f);
	}

	const glm::mat4& Camera::getProjectionMatrix() const
	{
		return m_projectionMatrix;
	}

	glm::vec3 Camera::GetPosition() const
	{
		return m_TransformComponent ? m_TransformComponent->Position : glm::vec3(0.0f);
	}

	glm::mat4 Camera::GetTransform() const
	{
		return m_TransformComponent ? m_TransformComponent->GetGlobalTransform() : glm::mat4(1.0f);
	}

	glm::vec3 Camera::GetFront() const
	{
		if (!m_TransformComponent)
			return glm::vec3(0, 0, -1);

		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat rotationQuat;
		glm::mat4 M = m_TransformComponent->GetGlobalTransform();

		if (glm::decompose(M, scale, rotationQuat, translation, skew, perspective))
		{
			glm::vec3 front = rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f);
			return glm::normalize(front);
		}
		
		return glm::vec3(0, 0, -1);
	}

	void Camera::updateProjectionMatrix()
	{
		const float w = std::max(1.0f, m_ViewportSize.x);
		const float h = std::max(1.0f, m_ViewportSize.y);
		const float aspect = w / h;

		switch (m_cameraType)
		{
		case CameraType::PERSPECTIVE:
		{
			m_projectionMatrix = glm::perspectiveRH_ZO(glm::radians(GetFov()), aspect, m_nearZ, m_farZ);
			break;
		}
		case CameraType::ORTHOGRAPHIC:
		{
			const float halfHeight = 0.5f;
			const float halfWidth = halfHeight * aspect;
			m_projectionMatrix = glm::orthoRH_ZO(-halfWidth, +halfWidth, -halfHeight, +halfHeight, m_nearZ, m_farZ);
			break;
		}
		default:
			m_projectionMatrix = glm::mat4(1.0f);
			break;
		}
	}

	float Camera::GetFov() const
	{
		return m_fov;
	}

	void Camera::SetFov(float fov)
	{
		m_fov = glm::clamp(fov, m_minFov, m_maxFov);

		updateProjectionMatrix();
	}

	float Camera::GetNearZ() const
	{
		return m_nearZ;
	}

	void Camera::SetNearZ(float nearZ)
	{
		m_nearZ = nearZ;
	}

	float Camera::GetFarZ() const
	{
		return m_farZ;
	}

	void Camera::SetFarZ(float farZ)
	{
		m_farZ = farZ;
	}

	void Camera::OnResize(float width, float height)
	{
		m_ViewportSize.x = width;
		m_ViewportSize.y = height;
		
		updateProjectionMatrix();
	}*/
}