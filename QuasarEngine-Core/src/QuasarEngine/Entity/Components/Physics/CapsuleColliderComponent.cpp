#include "qepch.h"
#include "CapsuleColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
	CapsuleColliderComponent::CapsuleColliderComponent()
	{
	}

	CapsuleColliderComponent::~CapsuleColliderComponent()
	{
		Entity entity{entt_entity, registry };
		if (entity.HasComponent<RigidBodyComponent>())
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
		}
	}

	void CapsuleColliderComponent::Init()
	{
		Entity entity{entt_entity, registry };
		auto& tc = entity.GetComponent<TransformComponent>();
		glm::vec3 entityScale = tc.Scale;
		
		if (entity.HasComponent<RigidBodyComponent>())
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
		}
	}

	void CapsuleColliderComponent::UpdateColliderMaterial()
	{
		
	}

	void CapsuleColliderComponent::UpdateColliderSize()
	{
		Entity entity{entt_entity, registry };
		if (entity.HasComponent<RigidBodyComponent>())
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
		}
	}
}