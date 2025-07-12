#include "qepch.h"
#include "DistanceJoint.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include <glm/gtx/transform.hpp>

namespace QuasarEngine {

    DistanceJoint::DistanceJoint(RigidBody* a, RigidBody* b,
        const glm::vec3& anchorA, const glm::vec3& anchorB)
        : Joint(a, b), localAnchorA(anchorA - a->GetPosition()), localAnchorB(anchorB - b->GetPosition()) {
        targetDistance = glm::length(anchorB - anchorA);
    }

    void DistanceJoint::PreSolve(float /*dt*/) {
        worldAnchorA = bodyA->GetPosition() + localAnchorA;
        worldAnchorB = bodyB->GetPosition() + localAnchorB;
    }

    void DistanceJoint::SolveConstraint() {
        glm::vec3 delta = worldAnchorB - worldAnchorA;
        float dist = glm::length(delta);

        if (!std::isfinite(dist) || dist < 1e-6f || dist > 1000.0f)
            return;

        glm::vec3 dir = delta / dist;
        float invMassA = bodyA->IsDynamic() ? bodyA->inverseMass : 0.0f;
        float invMassB = bodyB->IsDynamic() ? bodyB->inverseMass : 0.0f;
        float totalInvMass = invMassA + invMassB;

        if (totalInvMass < 1e-6f)
            return;

        // --- Calcul de la vitesse relative projetée sur l’axe
        glm::vec3 vA = bodyA->linearVelocity;
        glm::vec3 vB = bodyB->linearVelocity;
        float vRel = glm::dot(vB - vA, dir);

        // --- Bias pour corriger la distance cible (type Baumgarte)
        float bias = (dist - targetDistance) * 0.2f;

        float lambda = -(vRel + bias) / totalInvMass;

        // --- Appliquer l’impulsion sur les vitesses
        if (bodyA->IsDynamic())
            bodyA->linearVelocity += dir * lambda * invMassA;

        if (bodyB->IsDynamic())
            bodyB->linearVelocity -= dir * lambda * invMassB;
    }

}