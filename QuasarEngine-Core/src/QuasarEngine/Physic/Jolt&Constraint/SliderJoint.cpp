#include "qepch.h"
#include "SliderJoint.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace QuasarEngine {

    SliderJoint::SliderJoint(RigidBody* A, RigidBody* B, const glm::vec3& anchor, const glm::vec3& axis)
        : Joint(A, B)
    {
        glm::mat4 transformA = glm::translate(A->position) * glm::toMat4(A->orientation);
        glm::mat4 transformB = glm::translate(B->position) * glm::toMat4(B->orientation);

        mLocalAnchorA = glm::inverse(transformA) * glm::vec4(anchor, 1.0f);
        mLocalAnchorB = glm::inverse(transformB) * glm::vec4(anchor, 1.0f);

        mLocalAxisA = glm::normalize(glm::mat3(glm::inverse(transformA)) * axis);
    }

    void SliderJoint::PreSolve(float dt) {
        if (!mBodyA || !mBodyB) return;

        glm::mat4 transformA = glm::translate(mBodyA->position) * glm::toMat4(mBodyA->orientation);
        glm::mat4 transformB = glm::translate(mBodyB->position) * glm::toMat4(mBodyB->orientation);

        mWorldAnchorA = glm::vec3(transformA * glm::vec4(mLocalAnchorA, 1.0f));
        mWorldAnchorB = glm::vec3(transformB * glm::vec4(mLocalAnchorB, 1.0f));

        mWorldAxis = glm::normalize(glm::mat3(transformA) * mLocalAxisA);

        glm::vec3 error = mWorldAnchorB - mWorldAnchorA;
        glm::vec3 axisError = glm::dot(error, mWorldAxis) * mWorldAxis;
        glm::vec3 orthoError = error - axisError;

        mBiasTranslation = -(mBeta / dt) * orthoError;
        mAccumulatedImpulseTranslation = glm::vec3(0.0f);
    }

    void SliderJoint::SolveConstraint() {
        if (!mBodyA || !mBodyB) return;

        glm::vec3 ra = mWorldAnchorA - mBodyA->position;
        glm::vec3 rb = mWorldAnchorB - mBodyB->position;

        glm::vec3 va = mBodyA->linearVelocity + glm::cross(mBodyA->angularVelocity, ra);
        glm::vec3 vb = mBodyB->linearVelocity + glm::cross(mBodyB->angularVelocity, rb);
        glm::vec3 relativeVelocity = vb - va;

        glm::vec3 axisRelative = glm::dot(relativeVelocity, mWorldAxis) * mWorldAxis;
        glm::vec3 orthoRelative = relativeVelocity - axisRelative;

        glm::vec3 impulse = -(orthoRelative + mBiasTranslation);
        float invMass = mBodyA->inverseMass + mBodyB->inverseMass;

        if (invMass > 0.0f) {
            impulse /= invMass;

            mBodyA->linearVelocity -= impulse * mBodyA->inverseMass;
            mBodyB->linearVelocity += impulse * mBodyB->inverseMass;
        }

        mAccumulatedImpulseTranslation += impulse;

        // ----- Vérification limite de distance le long de l’axe -----
        glm::vec3 error = mWorldAnchorB - mWorldAnchorA;
        float currentDist = glm::dot(error, mWorldAxis);

        if (currentDist < mMinLimit || currentDist > mMaxLimit) {
            float clampedDist = glm::clamp(currentDist, mMinLimit, mMaxLimit);
            float correction = clampedDist - currentDist;

            glm::vec3 correctiveImpulse = (correction / invMass) * mWorldAxis;

            mBodyA->linearVelocity -= correctiveImpulse * mBodyA->inverseMass;
            mBodyB->linearVelocity += correctiveImpulse * mBodyB->inverseMass;
        }

        // ----- Blocage de la rotation -----
        mBodyA->angularVelocity = glm::vec3(0.0f);
        mBodyB->angularVelocity = glm::vec3(0.0f);
    }

    void SliderJoint::PostSolve() {
        std::cout << "[DEBUG] SliderJoint PostSolve called." << std::endl;
        float currentDist = glm::dot(mWorldAnchorB - mWorldAnchorA, mWorldAxis);
        std::cout << "[SliderJoint::PostSolve] Distance actuelle le long de l'axe : " << currentDist << std::endl;
        std::cout << "[SliderJoint::PostSolve] Impulsion accumulée (hors axe) : "
            << mAccumulatedImpulseTranslation.x << ", "
            << mAccumulatedImpulseTranslation.y << ", "
            << mAccumulatedImpulseTranslation.z << std::endl;

        std::cout << "[DEBUG] SliderJoint::PostSolve called for " << this << std::endl;
    }
}