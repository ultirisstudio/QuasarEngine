#include "qepch.h"
#include "SceneObject.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Scene/Camera.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>

namespace QuasarEngine
{
	SceneObject::SceneObject()
	{
		m_Scene = std::make_unique<Scene>();
	}

	void SceneObject::CreateScene()
	{
		m_Scene = std::make_unique<Scene>();
	}
}