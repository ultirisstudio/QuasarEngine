#include "qepch.h"
#include "FixedJoint.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace QuasarEngine {

    FixedJoint::FixedJoint(RigidBody* A, RigidBody* B)
        : Joint(A, B), mBeta(0.3f), mAngularStiffness(10.0f), mAccumulatedImpulseTranslation(0.0f)
    {
        glm::mat4 transformA = glm::translate(glm::mat4(1.0f), A->position) * glm::toMat4(A->orientation);
        glm::mat4 transformB = glm::translate(glm::mat4(1.0f), B->position) * glm::toMat4(B->orientation);

        glm::vec3 worldAnchor = A->position;

        mLocalAnchorA = glm::inverse(transformA) * glm::vec4(worldAnchor, 1.0f);
        mLocalAnchorB = glm::inverse(transformB) * glm::vec4(worldAnchor, 1.0f);

        mInitialRelativeOrientation = glm::inverse(A->orientation) * B->orientation;
    }

    FixedJoint::FixedJoint(RigidBody* A, RigidBody* B, const glm::vec3& anchor)
        : Joint(A, B), mBeta(0.3f), mAngularStiffness(10.0f), mAccumulatedImpulseTranslation(0.0f)
    {
        glm::mat4 transformA = glm::translate(glm::mat4(1.0f), A->position) * glm::toMat4(A->orientation);
        glm::mat4 transformB = glm::translate(glm::mat4(1.0f), B->position) * glm::toMat4(B->orientation);

        mLocalAnchorA = glm::inverse(transformA) * glm::vec4(anchor, 1.0f);
        mLocalAnchorB = glm::inverse(transformB) * glm::vec4(anchor, 1.0f);

        mInitialRelativeOrientation = glm::inverse(A->orientation) * B->orientation;
    }

    void FixedJoint::PreSolve(float dt) {
        if (!mBodyA || !mBodyB) return;

        glm::mat4 transformA = glm::translate(glm::mat4(1.0f), mBodyA->position) * glm::toMat4(mBodyA->orientation);
        glm::mat4 transformB = glm::translate(glm::mat4(1.0f), mBodyB->position) * glm::toMat4(mBodyB->orientation);

        glm::vec3 worldAnchorA = glm::vec3(transformA * glm::vec4(mLocalAnchorA, 1.0f));
        glm::vec3 worldAnchorB = glm::vec3(transformB * glm::vec4(mLocalAnchorB, 1.0f));

        glm::vec3 error = worldAnchorB - worldAnchorA;

        mBiasTranslation = -(mBeta / dt) * error;
        mAccumulatedImpulseTranslation = glm::vec3(0.0f);
    }

    void FixedJoint::SolveConstraint() {
        if (!mBodyA || !mBodyB) return;

        glm::mat4 transformA = glm::translate(glm::mat4(1.0f), mBodyA->position) * glm::toMat4(mBodyA->orientation);
        glm::mat4 transformB = glm::translate(glm::mat4(1.0f), mBodyB->position) * glm::toMat4(mBodyB->orientation);

        glm::vec3 worldAnchorA = glm::vec3(transformA * glm::vec4(mLocalAnchorA, 1.0f));
        glm::vec3 worldAnchorB = glm::vec3(transformB * glm::vec4(mLocalAnchorB, 1.0f));

        glm::vec3 ra = worldAnchorA - mBodyA->position;
        glm::vec3 rb = worldAnchorB - mBodyB->position;

        glm::vec3 va = mBodyA->linearVelocity + glm::cross(mBodyA->angularVelocity, ra);
        glm::vec3 vb = mBodyB->linearVelocity + glm::cross(mBodyB->angularVelocity, rb);
        glm::vec3 relativeVelocity = vb - va;

        glm::vec3 impulse = -(relativeVelocity + mBiasTranslation);
        impulse /= (mBodyA->inverseMass + mBodyB->inverseMass);

        mBodyA->linearVelocity -= impulse * mBodyA->inverseMass;
        mBodyB->linearVelocity += impulse * mBodyB->inverseMass;

        mAccumulatedImpulseTranslation += impulse;

        // --- Rotation fixée (avec couple correctif) ---
        glm::quat currentRelOrientation = glm::inverse(mBodyA->orientation) * mBodyB->orientation;
        glm::quat errorQuat = currentRelOrientation * glm::inverse(mInitialRelativeOrientation);

        if (glm::length(errorQuat) > 1e-6f && errorQuat.w < 1.0f) {
            float angle = 2.0f * acos(glm::clamp(errorQuat.w, -1.0f, 1.0f));
            glm::vec3 axis = glm::normalize(glm::vec3(errorQuat.x, errorQuat.y, errorQuat.z));

            glm::vec3 torque = axis * angle * mAngularStiffness;

            mBodyA->angularVelocity -= torque * mBodyA->inverseMass;
            mBodyB->angularVelocity += torque * mBodyB->inverseMass;
        }
    }

}