#include "qepch.h"
#include "PhysicEngine.h"
#include "RigidBody.h"
#include "Collision/CollisionDetection.h"
#include "Collision/ContactManifold.h"
#include "Collision/Collider.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <QuasarEngine/Physic/Jolt&Constraint/Joint.h>
#include "Jolt&Constraint/FixedJoint.h"
#include "Jolt&Constraint/HingeJoint.h"
#include "Jolt&Constraint/SliderJoint.h"


namespace QuasarEngine {

	struct PhysicEngineData {
		glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
		std::vector<RigidBody*> bodies;
		std::vector<ContactManifold> contacts;
		std::vector<Joint*> joints;

	};

	static PhysicEngineData* s_Data = nullptr;

	void PhysicEngine::Init() {
		s_Data = new PhysicEngineData();
		CollisionDetection::Init();
	}

	void PhysicEngine::Shutdown() {
		delete s_Data;
		s_Data = nullptr;
	}

	void PhysicEngine::RegisterJoint(Joint* joint) {
		if (s_Data && joint) {
			std::cout << "[DEBUG] Joint enregistré : " << joint << std::endl;
			s_Data->joints.push_back(joint);
		}
	}


	/*
	void PhysicEngine::Update(double dt) {
		if (!s_Data) return;

		std::vector<ContactManifold> contacts;

		for (RigidBody* body : s_Data->bodies) {
			if (!body || !body->IsDynamic() || !body->UsesGravity()) continue;

			glm::vec3 gravityForce = s_Data->gravity * body->mass;
			body->ApplyForce(gravityForce);
		}

		for (size_t i = 0; i < s_Data->bodies.size(); ++i) {
			for (size_t j = i + 1; j < s_Data->bodies.size(); ++j) {
				RigidBody* A = s_Data->bodies[i];
				RigidBody* B = s_Data->bodies[j];
				if (!A || !B) continue;

				if (A->type == RigidBodyType::Static && B->type == RigidBodyType::Static)
					continue;

				for (auto& colA : A->colliders) {
					for (auto& colB : B->colliders) {
						Collider* colliderA = colA;
						Collider* colliderB = colB;

						if (!colliderA || !colliderB) continue;

						ContactManifold manifold;
						if (CollisionDetection::CheckCollision(colliderA, colliderB, manifold)) {
							std::cout << "Collision" << std::endl;

							contacts.push_back(manifold);

							std::cout << "[COLLISION] Contact entre " << colliderA << " et " << colliderB << "\n";
							std::cout << "penetration: " << manifold.penetrationDepth << "\n";
							for (const auto& pt : manifold.contactPoints) {
								std::cout << "contact point: (" << pt.x << ", " << pt.y << ", " << pt.z << ")\n";
							}
						}
					}
				}
			}
		}

		for (int k = 0; k < 8; ++k) {
			for (const auto& contact : contacts) {
				ResolveCollision(contact);
			}
		}

		for (RigidBody* body : s_Data->bodies) {
			if (body)
				body->Integrate(static_cast<float>(dt));
		}
	}*/

	/*
	void PhysicEngine::Update(double dt) {
		if (!s_Data) return;

		std::vector<ContactManifold> contacts;

		for (RigidBody* body : s_Data->bodies) {
			if (!body || !body->IsDynamic() || !body->UsesGravity()) continue;

			glm::vec3 gravityForce = s_Data->gravity * body->mass;
			body->ApplyForce(gravityForce);
		}

		for (size_t i = 0; i < s_Data->bodies.size(); ++i) {
			for (size_t j = i + 1; j < s_Data->bodies.size(); ++j) {
				RigidBody* A = s_Data->bodies[i];
				RigidBody* B = s_Data->bodies[j];
				if (!A || !B) continue;

				if (A->type == RigidBodyType::Static && B->type == RigidBodyType::Static)
					continue;

				for (auto& colA : A->colliders) {
					for (auto& colB : B->colliders) {
						Collider* colliderA = colA;
						Collider* colliderB = colB;

						if (!colliderA || !colliderB) continue;

						ContactManifold manifold;
						if (CollisionDetection::CheckCollision(colliderA, colliderB, manifold)) {
							std::cout << "Collision" << std::endl;

							contacts.push_back(manifold);

							std::cout << "[COLLISION] Contact entre " << colliderA << " et " << colliderB << "\n";
							std::cout << "penetration: " << manifold.penetrationDepth << "\n";
							for (const auto& pt : manifold.contactPoints) {
								std::cout << "contact point: (" << pt.x << ", " << pt.y << ", " << pt.z << ")\n";
							}
						}
					}
				}
			}
		}

		for (int k = 0; k < 8; ++k) {
			for (const auto& contact : contacts) {
				ResolveCollision(contact);
			}
		}

		for (RigidBody* body : s_Data->bodies) {
			if (body)
				body->Integrate(static_cast<float>(dt));
		}
	}*/

	void PhysicEngine::Update(double dt) {
		if (!s_Data) return;

		// ----------------------
		// 1. Appliquer la gravité
		// ----------------------
		for (RigidBody* body : s_Data->bodies) {
			if (!body || !body->IsDynamic() || !body->UsesGravity()) continue;

			glm::vec3 gravityForce = s_Data->gravity * body->mass;
			body->ApplyForce(gravityForce);
		}

		// ----------------------
		// 2. Détection de collisions
		// ----------------------
		std::vector<ContactManifold> contacts;

		for (size_t i = 0; i < s_Data->bodies.size(); ++i) {
			for (size_t j = i + 1; j < s_Data->bodies.size(); ++j) {
				RigidBody* A = s_Data->bodies[i];
				RigidBody* B = s_Data->bodies[j];
				if (!A || !B) continue;

				if (A->type == RigidBodyType::Static && B->type == RigidBodyType::Static)
					continue;

				for (auto& colA : A->colliders) {
					for (auto& colB : B->colliders) {
						Collider* colliderA = colA;
						Collider* colliderB = colB;

						if (!colliderA || !colliderB) continue;

						ContactManifold manifold;
						if (CollisionDetection::CheckCollision(colliderA, colliderB, manifold)) {
							contacts.push_back(manifold);
						}
					}
				}
			}
		}

		// ----------------------
		// 3. Préparation des contraintes de joint
		// ----------------------
		for (Joint* joint : s_Data->joints)
			joint->PreSolve(static_cast<float>(dt));

		// ----------------------
		// 4. Résolution des collisions et contraintes
		// ----------------------
		const int solverIterations = 8;
		for (int k = 0; k < solverIterations; ++k) {

			// Collisions
			for (const auto& contact : contacts) {
				ResolveCollision(contact);
			}

			// Joints
			for (Joint* joint : s_Data->joints) {
				joint->SolveConstraint();
			}
		}

		// ----------------------
		// 5. Intégration des mouvements
		// ----------------------
		for (RigidBody* body : s_Data->bodies) {
			if (body)
				body->Integrate(static_cast<float>(dt));
		}

		// ----------------------
		// 6. Debug Post-Solve
		// ----------------------
		for (Joint* joint : s_Data->joints)
			joint->PostSolve();
	}


	void PhysicEngine::Reload() {
		Init();
		Update(0.0);
	}

	void PhysicEngine::RegisterRigidBody(RigidBody* body) {
		if (!s_Data || !body) return;
		s_Data->bodies.push_back(body);
	}

	/*void PhysicEngine::ResolveCollision(const ContactManifold& contact) {
		RigidBody* A = contact.colliderA->GetAttachedRigidBody();
		RigidBody* B = contact.colliderB->GetAttachedRigidBody();
		if (!A || !B) return;

		bool isAStatic = (A->type == RigidBodyType::Static);
		bool isBStatic = (B->type == RigidBodyType::Static);
		if (isAStatic && isBStatic) return;

		float invMassA = isAStatic ? 0.0f : A->inverseMass;
		float invMassB = isBStatic ? 0.0f : B->inverseMass;
		float totalInvMass = invMassA + invMassB;

		if (totalInvMass <= 0.0f || contact.contactPoints.empty()) return;

		glm::vec3 normal = glm::normalize(contact.contactNormal);
		float restitution = 0.3f;  // rebond
		float friction = 0.4f;     // friction tangentielle

		for (const glm::vec3& contactPoint : contact.contactPoints) {
			glm::vec3 ra = contactPoint - A->GetPosition();
			glm::vec3 rb = contactPoint - B->GetPosition();

			glm::vec3 va = A->linearVelocity + glm::cross(A->angularVelocity, ra);
			glm::vec3 vb = B->linearVelocity + glm::cross(B->angularVelocity, rb);
			glm::vec3 relativeVelocity = vb - va;

			float velAlongNormal = glm::dot(relativeVelocity, normal);
			if (velAlongNormal > 0.0f) continue;

			glm::vec3 raCrossN = glm::cross(ra, normal);
			glm::vec3 rbCrossN = glm::cross(rb, normal);

			float angularEffectA = isAStatic ? 0.0f : glm::dot(normal, glm::cross(A->inverseInertiaTensorWorld * raCrossN, ra));
			float angularEffectB = isBStatic ? 0.0f : glm::dot(normal, glm::cross(B->inverseInertiaTensorWorld * rbCrossN, rb));

			float denom = totalInvMass + angularEffectA + angularEffectB;
			if (denom <= 0.0f) continue;

			float j = -(1.0f + restitution) * velAlongNormal / denom;
			j /= static_cast<float>(contact.contactPoints.size());

			glm::vec3 impulse = j * normal;

			if (!isAStatic) {
				A->linearVelocity -= impulse * invMassA;
				A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, impulse);
			}
			if (!isBStatic) {
				B->linearVelocity += impulse * invMassB;
				B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, impulse);
			}

			// --------- FRICTION ---------
			glm::vec3 tangent = relativeVelocity - normal * velAlongNormal;
			if (glm::length(tangent) > 1e-6f) {
				tangent = glm::normalize(tangent);
				float jt = -glm::dot(relativeVelocity, tangent) / denom;
				jt /= static_cast<float>(contact.contactPoints.size());

				glm::vec3 frictionImpulse = jt * tangent * friction;

				if (!isAStatic) {
					A->linearVelocity -= frictionImpulse * invMassA;
					A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, frictionImpulse);
				}
				if (!isBStatic) {
					B->linearVelocity += frictionImpulse * invMassB;
					B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, frictionImpulse);
				}
			}
		}

		// --------- POSITIONAL CORRECTION ---------
		const float slop = 0.005f;     // tolérance minimale de pénétration
		const float percent = 0.6f;    // pour éviter les oscillations

		float penetration = glm::max(contact.penetrationDepth - slop, 0.0f);
		glm::vec3 correction = (penetration / totalInvMass) * percent * normal;

		if (!isAStatic) A->position -= correction * invMassA;
		if (!isBStatic) B->position += correction * invMassB;
	}*/

	/*void PhysicEngine::ResolveCollision(const ContactManifold& contact) {
		RigidBody* A = contact.colliderA->GetAttachedRigidBody();
		RigidBody* B = contact.colliderB->GetAttachedRigidBody();
		if (!A || !B) return;

		bool isAStatic = (A->type == RigidBodyType::Static);
		bool isBStatic = (B->type == RigidBodyType::Static);
		if (isAStatic && isBStatic) return;

		float invMassA = isAStatic ? 0.0f : A->inverseMass;
		float invMassB = isBStatic ? 0.0f : B->inverseMass;
		float totalInvMass = invMassA + invMassB;

		if (totalInvMass <= 0.0f || contact.contactPoints.empty()) return;

		glm::vec3 normal = glm::normalize(contact.contactNormal);
		float restitution = 0.3f;  // rebond
		float friction = 0.4f;     // friction tangentielle

		for (const glm::vec3& contactPoint : contact.contactPoints) {
			glm::vec3 ra = contactPoint - A->GetPosition();
			glm::vec3 rb = contactPoint - B->GetPosition();

			glm::vec3 va = A->linearVelocity + glm::cross(A->angularVelocity, ra);
			glm::vec3 vb = B->linearVelocity + glm::cross(B->angularVelocity, rb);
			glm::vec3 relativeVelocity = vb - va;

			float velAlongNormal = glm::dot(relativeVelocity, normal);
			if (velAlongNormal > 0.0f) continue;

			glm::vec3 raCrossN = glm::cross(ra, normal);
			glm::vec3 rbCrossN = glm::cross(rb, normal);

			float angularEffectA = isAStatic ? 0.0f : glm::dot(normal, glm::cross(A->inverseInertiaTensorWorld * raCrossN, ra));
			float angularEffectB = isBStatic ? 0.0f : glm::dot(normal, glm::cross(B->inverseInertiaTensorWorld * rbCrossN, rb));

			float denom = totalInvMass + angularEffectA + angularEffectB;
			if (denom <= 0.0f) continue;

			float j = -(1.0f + restitution) * velAlongNormal / denom;
			j /= static_cast<float>(contact.contactPoints.size());

			glm::vec3 impulse = j * normal;

			if (!isAStatic) {
				A->linearVelocity -= impulse * invMassA;
				A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, impulse);
			}
			if (!isBStatic) {
				B->linearVelocity += impulse * invMassB;
				B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, impulse);
			}

			// --------- FRICTION ---------
			glm::vec3 tangent = relativeVelocity - normal * velAlongNormal;
			if (glm::length(tangent) > 1e-6f) {
				tangent = glm::normalize(tangent);
				float jt = -glm::dot(relativeVelocity, tangent) / denom;
				jt /= static_cast<float>(contact.contactPoints.size());

				glm::vec3 frictionImpulse = jt * tangent * friction;

				if (!isAStatic) {
					A->linearVelocity -= frictionImpulse * invMassA;
					A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, frictionImpulse);
				}
				if (!isBStatic) {
					B->linearVelocity += frictionImpulse * invMassB;
					B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, frictionImpulse);
				}
			}
		}

		// --------- POSITIONAL CORRECTION ---------
		const float slop = 0.005f;     // tolérance minimale de pénétration
		const float percent = 0.6f;    // pour éviter les oscillations

		float penetration = glm::max(contact.penetrationDepth - slop, 0.0f);
		glm::vec3 correction = (penetration / totalInvMass) * percent * normal;

		if (!isAStatic) A->position -= correction * invMassA;
		if (!isBStatic) B->position += correction * invMassB;
	}*/

	void PhysicEngine::ResolveCollision(const ContactManifold& contact) {
		RigidBody* A = contact.colliderA->GetAttachedRigidBody();
		RigidBody* B = contact.colliderB->GetAttachedRigidBody();
		if (!A || !B) return;

		bool isAStatic = (A->type == RigidBodyType::Static);
		bool isBStatic = (B->type == RigidBodyType::Static);
		if (isAStatic && isBStatic) return;

		float invMassA = isAStatic ? 0.0f : A->inverseMass;
		float invMassB = isBStatic ? 0.0f : B->inverseMass;
		float totalInvMass = invMassA + invMassB;

		if (totalInvMass <= 0.0f || contact.contactPoints.empty()) return;

		glm::vec3 normal = glm::normalize(contact.contactNormal);
		float restitution = 0.2f;
		float friction = 0.4f;

		for (const glm::vec3& contactPoint : contact.contactPoints) {
			glm::vec3 ra = contactPoint - A->GetPosition();
			glm::vec3 rb = contactPoint - B->GetPosition();

			glm::vec3 va = A->linearVelocity + glm::cross(A->angularVelocity, ra);
			glm::vec3 vb = B->linearVelocity + glm::cross(B->angularVelocity, rb);
			glm::vec3 relativeVelocity = vb - va;

			float velAlongNormal = glm::dot(relativeVelocity, normal);
			if (velAlongNormal > 0.0f) continue;

			glm::vec3 raCrossN = glm::cross(ra, normal);
			glm::vec3 rbCrossN = glm::cross(rb, normal);

			float angularEffectA = isAStatic ? 0.0f : glm::dot(normal, glm::cross(A->inverseInertiaTensorWorld * raCrossN, ra));
			float angularEffectB = isBStatic ? 0.0f : glm::dot(normal, glm::cross(B->inverseInertiaTensorWorld * rbCrossN, rb));

			float denom = totalInvMass + angularEffectA + angularEffectB;
			if (denom <= 0.0f) continue;

			float j = -(1.0f + restitution) * velAlongNormal / denom;
			j /= static_cast<float>(contact.contactPoints.size());

			glm::vec3 impulse = j * normal;

			if (!isAStatic) {
				A->linearVelocity -= impulse * invMassA;
				A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, impulse);
			}

			if (!isBStatic) {
				B->linearVelocity += impulse * invMassB;
				B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, impulse);
			}

			// --------- FRICTION ---------
			glm::vec3 tangent = relativeVelocity - normal * velAlongNormal;
			if (glm::length(tangent) > 1e-6f) {
				tangent = glm::normalize(tangent);
				float jt = -glm::dot(relativeVelocity, tangent) / denom;
				jt /= static_cast<float>(contact.contactPoints.size());

				// Coulomb friction
				float mu = friction;
				jt = glm::clamp(jt, -j * mu, j * mu);

				glm::vec3 frictionImpulse = jt * tangent;

				if (!isAStatic) {
					A->linearVelocity -= frictionImpulse * invMassA;
					A->angularVelocity -= A->inverseInertiaTensorWorld * glm::cross(ra, frictionImpulse);
				}

				if (!isBStatic) {
					B->linearVelocity += frictionImpulse * invMassB;
					B->angularVelocity += B->inverseInertiaTensorWorld * glm::cross(rb, frictionImpulse);
				}
			}
		}

		// --------- POSITIONAL CORRECTION (douce) ---------
		const float slop = 0.01f;
		const float percent = 0.2f; // plus bas que 0.6 pour être stable avec des joints

		float penetration = glm::max(contact.penetrationDepth - slop, 0.0f);
		glm::vec3 correction = (penetration / totalInvMass) * percent * normal;

		if (!isAStatic) A->position -= correction * invMassA;
		if (!isBStatic) B->position += correction * invMassB;
	}

	void PhysicEngine::DestroyJoint(Joint* joint) {
		if (!s_Data || !joint) return;

		auto& joints = s_Data->joints;
		auto it = std::remove(joints.begin(), joints.end(), joint);
		if (it != joints.end()) {
			joints.erase(it, joints.end());
			delete joint;
		}
	}

	Joint* PhysicEngine::CreateJoint(const JointInfo& info) {
		if (!s_Data) return nullptr;

		Joint* joint = nullptr;

		switch (info.type) {
		case JointType::Fixed:
			joint = new FixedJoint(info.bodyA, info.bodyB, info.anchor);
			break;

		case JointType::Hinge:
			joint = new HingeJoint(info.bodyA, info.bodyB, info.anchor, info.axis);
			break;

		case JointType::Slider:
			joint = new SliderJoint(info.bodyA, info.bodyB, info.anchor, info.axis);
			static_cast<SliderJoint*>(joint)->SetLimits(info.minLimit, info.maxLimit);
			break;

		default:
			std::cerr << "[PhysicEngine] JointType non supporté !" << std::endl;
			return nullptr;
		}

		s_Data->joints.push_back(joint);
		return joint;
	}


	void PhysicEngine::Clear() {
		if (!s_Data) return;

		for (RigidBody* body : s_Data->bodies)
			delete body;
		s_Data->bodies.clear();

		for (Joint* joint : s_Data->joints)
			delete joint;
		s_Data->joints.clear();

		s_Data->contacts.clear();
	}

}