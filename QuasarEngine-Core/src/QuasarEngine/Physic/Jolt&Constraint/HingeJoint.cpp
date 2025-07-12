#include "qepch.h"
#include "HingeJoint.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace QuasarEngine {

    HingeJoint::HingeJoint(RigidBody* A, RigidBody* B, const glm::vec3& anchor, const glm::vec3& axis)
        : Joint(A, B)
    {
        glm::mat4 transformA = glm::translate(A->position) * glm::toMat4(A->orientation);
        glm::mat4 transformB = glm::translate(B->position) * glm::toMat4(B->orientation);

        mLocalAnchorA = glm::inverse(transformA) * glm::vec4(anchor, 1.0f);
        mLocalAnchorB = glm::inverse(transformB) * glm::vec4(anchor, 1.0f);

        mLocalAxisA = glm::normalize(glm::mat3(glm::inverse(transformA)) * axis);
        mLocalAxisB = glm::normalize(glm::mat3(glm::inverse(transformB)) * axis);
    }

    void HingeJoint::PreSolve(float dt) {
        if (!mBodyA || !mBodyB) return;

        glm::mat4 transformA = glm::translate(mBodyA->position) * glm::toMat4(mBodyA->orientation);
        glm::mat4 transformB = glm::translate(mBodyB->position) * glm::toMat4(mBodyB->orientation);

        mWorldAnchorA = glm::vec3(transformA * glm::vec4(mLocalAnchorA, 1.0f));
        mWorldAnchorB = glm::vec3(transformB * glm::vec4(mLocalAnchorB, 1.0f));

        mWorldAxis = glm::normalize(glm::vec3(transformA * glm::vec4(mLocalAxisA, 0.0f)));

        glm::vec3 error = mWorldAnchorB - mWorldAnchorA;
        mBiasTranslation = -(mBeta / dt) * error;

        mAccumulatedImpulseTranslation = glm::vec3(0.0f);
    }

    void HingeJoint::SolveConstraint() {
        if (!mBodyA || !mBodyB) return;

        glm::vec3 ra = mWorldAnchorA - mBodyA->position;
        glm::vec3 rb = mWorldAnchorB - mBodyB->position;

        glm::vec3 va = mBodyA->linearVelocity + glm::cross(mBodyA->angularVelocity, ra);
        glm::vec3 vb = mBodyB->linearVelocity + glm::cross(mBodyB->angularVelocity, rb);
        glm::vec3 relativeVelocity = vb - va;

        glm::vec3 impulse = -(relativeVelocity + mBiasTranslation);
        float invMass = mBodyA->inverseMass + mBodyB->inverseMass;

        if (invMass > 0.0f) {
            impulse /= invMass;

            mBodyA->linearVelocity -= impulse * mBodyA->inverseMass;
            mBodyB->linearVelocity += impulse * mBodyB->inverseMass;
        }

        mAccumulatedImpulseTranslation += impulse;

        if (mEnableLimits) {
            glm::vec3 axisA = glm::normalize(mBodyA->orientation * mLocalAxisA);
            glm::vec3 axisB = glm::normalize(mBodyB->orientation * mLocalAxisB);

            glm::vec3 refAxis = glm::normalize(glm::cross(mWorldAxis, glm::vec3(1, 0, 0)));
            if (glm::length(refAxis) < 1e-4f)
                refAxis = glm::normalize(glm::cross(mWorldAxis, glm::vec3(0, 1, 0)));

            glm::vec3 perpA = glm::normalize(axisA - glm::dot(axisA, mWorldAxis) * mWorldAxis);
            glm::vec3 perpB = glm::normalize(axisB - glm::dot(axisB, mWorldAxis) * mWorldAxis);

            float angle = atan2(glm::dot(glm::cross(perpA, perpB), mWorldAxis), glm::dot(perpA, perpB));

            if (angle < mMinAngle || angle > mMaxAngle) {
                float correction = 0.0f;
                if (angle < mMinAngle)
                    correction = angle - mMinAngle;
                else if (angle > mMaxAngle)
                    correction = angle - mMaxAngle;

                glm::vec3 correctiveTorque = -0.2f * correction * mWorldAxis;

                mBodyA->angularVelocity -= correctiveTorque * mBodyA->inverseMass;
                mBodyB->angularVelocity += correctiveTorque * mBodyB->inverseMass;
            }
        }
    }

}