#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>
#include <QuasarEngine/Physic/Collision/ContactManifold.h>

namespace QuasarEngine {

	class Collider;

	class BoxColliderComponent : public PrimitiveColliderComponent {
	public:
		BoxColliderComponent();
		~BoxColliderComponent();

		BoxColliderComponent(const BoxColliderComponent&) = delete;
		BoxColliderComponent& operator=(const BoxColliderComponent&) = delete;

		BoxColliderComponent(BoxColliderComponent&&) = default;
		BoxColliderComponent& operator=(BoxColliderComponent&&) = default;

		void Init() override;
		void UpdateColliderMaterial() override;
		void UpdateColliderSize() override;

		bool m_UseEntityScale = true;
		glm::vec3 m_Size = { 1.0f, 1.0f, 1.0f };

		bool& UseEntityScale() { return m_UseEntityScale; }
		glm::vec3& Size() { return m_Size; }
		float mass = 1.0f;
		float friction = 0.5f;
		float bounciness = 0.2f;

		float linearDamping = 0.01f;
		float angularDamping = 0.05f;

		Collider* GetCollider() const { return collider.get(); }

	private:
		std::unique_ptr<Collider> collider;

	};

}