#include "qepch.h"
#include "BoxColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/Collision/Collider.h>
#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/RigidBody.h>
#include <QuasarEngine/Physic/Collision/CollisionDetection.h>
#include <QuasarEngine/Physic/PhysicEngine.h>


namespace QuasarEngine {

	BoxColliderComponent::BoxColliderComponent() {}

	BoxColliderComponent::~BoxColliderComponent() {
		if (collider) {
			CollisionDetection::UnregisterCollider(collider.get());
		}
	}

	void BoxColliderComponent::Init() {
		Entity entity{ entt_entity, registry };
		auto& tc = entity.GetComponent<TransformComponent>();

		if (entity.HasComponent<RigidBodyComponent>()) {
			auto& rb = entity.GetComponent<RigidBodyComponent>();

			glm::vec3 size = m_UseEntityScale ? tc.Scale : m_Size;
			auto shape = std::make_unique<BoxShape>(size);

			collider = std::make_unique<Collider>(rb.GetRigidBody());
			collider->AddShape(std::move(shape));

			rb.GetRigidBody()->AddCollider(collider.get());

			if (collider) {
				CollisionDetection::RegisterCollider(collider.get());
				std::cout << "[DEBUG] Collider enregistré : " << collider.get() << std::endl;

				UpdateColliderMaterial();
			}
		}
	}

	void BoxColliderComponent::UpdateColliderMaterial()
	{
		if (!collider.get()) return;

		auto& shapes = collider->GetProxyShapes();
		for (auto& shape : shapes)
		{
			shape->SetFriction(friction);
			shape->SetBounciness(bounciness);
		}

		Entity entity{ entt_entity, registry };
		if (entity.HasComponent<RigidBodyComponent>())
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (rb.GetRigidBody())
			{
				rb.GetRigidBody()->mass = mass;
				rb.GetRigidBody()->inverseMass = (mass > 0.0f) ? 0.5f / mass : 0.0f;
			}
		}
	}

	void BoxColliderComponent::UpdateColliderSize() {
		if (!collider.get()) return;

		Entity entity{ entt_entity, registry };
		glm::vec3 scale = m_UseEntityScale ?
			entity.GetComponent<TransformComponent>().Scale :
			m_Size;

		// Pour simplifier : on efface la shape précédente si une seule
		auto& shapes = collider->GetProxyShapes();
		if (!shapes.empty()) {
			shapes.clear();  // On libère la forme actuelle (à améliorer plus tard pour multi-shape)
		}

		// Nouvelle forme avec taille mise à jour
		std::unique_ptr<BoxShape> newShape = std::make_unique<BoxShape>(scale);

		// Ré-attache la nouvelle shape
		collider->AddShape(std::move(newShape));
	}

}