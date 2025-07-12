#include "qepch.h"
#include "RigidBodyComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Physic/RigidBody.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Physic/Collision/Collider.h>

namespace QuasarEngine {

	RigidBodyComponent::RigidBodyComponent() {}

	RigidBodyComponent::~RigidBodyComponent() {
		Destroy();
	}

	void RigidBodyComponent::Destroy() {
		rigidBody.reset();
	}

	void RigidBodyComponent::Init() {
		Entity entity{ entt_entity, registry };
		auto& tc = entity.GetComponent<TransformComponent>();

		glm::vec3 pos = tc.Position;
		glm::quat rot = glm::quat(tc.Rotation);

		float mass = (bodyTypeString == "DYNAMIC") ? 1.0f : 0.0f;
		rigidBody = std::make_unique<RigidBody>(pos, rot, mass);


		UpdateBodyType();
		UpdateLinearAxisFactor();
		UpdateAngularAxisFactor();
		UpdateEnableGravity();  // Important pour bien initialiser l'état
		UpdateDamping();

		// Enregistrement dans le moteur physique
		PhysicEngine::RegisterRigidBody(rigidBody.get());

	}

	void RigidBodyComponent::Update(float dt) {
		if (!rigidBody) return;

		Entity entity{ entt_entity, registry };
		auto& tc = entity.GetComponent<TransformComponent>();

		tc.Position = rigidBody->GetPosition();
		tc.Rotation = glm::eulerAngles(rigidBody->GetOrientation());
	}

	void RigidBodyComponent::UpdateEnableGravity() {
		if (!rigidBody) return;
		rigidBody->SetEnableGravity(enableGravity);
	}

	void RigidBodyComponent::UpdateDamping() {
		if (!rigidBody) return;
		rigidBody->linearDamping = linearDamping;
		rigidBody->angularDamping = angularDamping;
	}

	void RigidBodyComponent::UpdateBodyType() {
		if (!rigidBody) return;

		if (bodyTypeString == "DYNAMIC") {
			rigidBody->type = RigidBodyType::Dynamic;
		}
		else if (bodyTypeString == "STATIC") {
			rigidBody->type = RigidBodyType::Static;
		}
		else if (bodyTypeString == "KINEMATIC") {
			rigidBody->type = RigidBodyType::Kinematic;
		}
		else {
			std::cout << "Invalid body type: " << bodyTypeString << std::endl;
		}
	}

	void RigidBodyComponent::UpdateLinearAxisFactor() {
		// À implémenter plus tard dans RigidBody (lock des vitesses)
	}

	void RigidBodyComponent::UpdateAngularAxisFactor() {
		// À implémenter plus tard dans RigidBody (lock des rotations)
	}

}