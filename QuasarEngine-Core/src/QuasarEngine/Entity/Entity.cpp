#include "qepch.h"
#include "Entity.h"

//#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
	Entity::Entity(entt::entity handle, Registry* registry)
		: m_EntityHandle(handle), m_Registry(registry)
	{
	}

	/*void Entity::Destroy()
	{
		if (HasComponent<QuasarEngine::RigidBodyComponent>())
		{
			GetComponent<QuasarEngine::RigidBodyComponent>().Destroy();
		}
	}*/
}