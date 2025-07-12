#pragma once
#include <vector>
#include "QuasarEngine/Physic/RigidBody.h"
#include "QuasarEngine/Physic/Collision/Collider.h"
#include "QuasarEngine/Physic/Collision/CollisionDetection.h"
#include "QuasarEngine/Physic/Jolt&Constraint/Joint.h"


namespace QuasarEngine
{

	struct JointInfo {
		RigidBody* bodyA = nullptr;
		RigidBody* bodyB = nullptr;
		JointType type;
		glm::vec3 anchor = glm::vec3(0.0f);
		glm::vec3 axis = glm::vec3(0, 1, 0);
		float minLimit = 0.0f;
		float maxLimit = 0.0f;
		float stiffness = 1.0f;
	};


	class PhysicEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void Update(double dt);

		static void Reload();

		static void RegisterRigidBody(RigidBody* body);
		static void ResolveCollision(const ContactManifold& contact);

		static void DestroyJoint(Joint* joint);
		static void RegisterJoint(Joint* joint);

		static Joint* CreateJoint(const JointInfo& info);
		static void Clear();

	private:
		static std::vector<Joint*> s_Joints;
	};
}