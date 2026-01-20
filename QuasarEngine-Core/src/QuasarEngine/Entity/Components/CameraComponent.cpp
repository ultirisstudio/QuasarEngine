#include "qepch.h"
#include "CameraComponent.h"
#include <glm/gtc/matrix_transform.hpp>

#include <glm/ext/matrix_clip_space.hpp>

namespace QuasarEngine
{
	void CameraComponent::RebuildProjection() {
		const float aspect = Aspect();

		switch (Type) {
		case CameraType::Perspective: {
			m_projection = glm::perspectiveRH_ZO(glm::radians(FovDeg), aspect, NearZ, FarZ);
			break;
		}
		case CameraType::Orthographic: {
			const float halfH = 0.5f * OrthoHeight;
			const float halfW = halfH * aspect;
			m_projection = glm::orthoRH_ZO(-halfW, +halfW, -halfH, +halfH, NearZ, FarZ);
			break;
		}
		default:
			m_projection = glm::mat4(1.0f);
			break;
		}
	}
}

/*#include "qepch.h"
#include "CameraComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>

namespace QuasarEngine
{
	CameraComponent::CameraComponent()
	{
		m_Camera = std::make_unique<Camera>();
	}

	void CameraComponent::setType(CameraType type)
	{
		switch (type)
		{
		case CameraType::PERSPECTIVE:
			item_type = "Perspective";
			m_Camera->m_cameraType = CameraType::PERSPECTIVE;
			m_Camera->updateProjectionMatrix();
			break;
		case CameraType::ORTHOGRAPHIC:
			item_type = "Orthographic";
			m_Camera->m_cameraType = CameraType::ORTHOGRAPHIC;
			m_Camera->updateProjectionMatrix();
			break;
		default:
			item_type = "Perspective";
			m_Camera->m_cameraType = CameraType::PERSPECTIVE;
			m_Camera->updateProjectionMatrix();
			break;
		}
	}
}*/