#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include <QuasarEngine/Physic/RigidBody.h>
#include <QuasarEngine/Physic/Collision/Collider.h>

namespace QuasarEngine {

	class RigidBodyComponent : public Component {
	public:
		std::string bodyTypeString = "DYNAMIC";
		bool enableGravity = true;

		bool m_LinearAxisFactorX = true;
		bool m_LinearAxisFactorY = true;
		bool m_LinearAxisFactorZ = true;

		bool m_AngularAxisFactorX = true;
		bool m_AngularAxisFactorY = true;
		bool m_AngularAxisFactorZ = true;

		float linearDamping = 0.01f;
		float angularDamping = 0.05f;

		RigidBodyComponent();
		~RigidBodyComponent() override;

		RigidBodyComponent(const RigidBodyComponent&) = delete;
		RigidBodyComponent& operator=(const RigidBodyComponent&) = delete;

		RigidBodyComponent(RigidBodyComponent&&) = default;
		RigidBodyComponent& operator=(RigidBodyComponent&&) = default;

		void Destroy();
		void Init();
		void Update(float dt);

		void UpdateEnableGravity();
		void UpdateBodyType();
		void UpdateLinearAxisFactor();
		void UpdateAngularAxisFactor();
		void UpdateDamping();

		RigidBody* GetRigidBody() { return rigidBody.get(); }
		Collider* GetCollider() const { return m_Collider.get(); }

	private:
		std::unique_ptr<RigidBody> rigidBody;
		std::unique_ptr<Collider> m_Collider;
	};

}
