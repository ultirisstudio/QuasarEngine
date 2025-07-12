#include "qepch.h"
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
}