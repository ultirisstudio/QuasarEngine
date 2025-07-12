#pragma once

#include "Joint.h"
#include <glm/glm.hpp>

namespace QuasarEngine {

    class SliderJoint : public Joint {
    public:
        SliderJoint(RigidBody* bodyA, RigidBody* bodyB, const glm::vec3& anchor, const glm::vec3& axis);

        void PreSolve(float dt) override;
        void SolveConstraint() override;

        //virtual JointType GetType() const override { return JointType::Slider; }

        JointType GetType() const override { return JointType::Slider; }

        void SetLimits(float lower, float upper) {
            mLowerLimit = lower;
            mUpperLimit = upper;
        }

        void EnableLimit(bool enable) {
            mLimitEnabled = enable;
        }

        void LockRotation(bool lock) {
            mLockRotation = lock;
        }
    private:
        glm::vec3 mLocalAnchorA;
        glm::vec3 mLocalAnchorB;
        glm::vec3 mLocalAxisA;

        glm::vec3 mWorldAnchorA;
        glm::vec3 mWorldAnchorB;
        glm::vec3 mWorldAxis;

        glm::vec3 mBiasTranslation;
        glm::vec3 mAccumulatedImpulseTranslation = glm::vec3(0.0f);

        float mMinLimit = -1.0f;
        float mMaxLimit = 1.0f;
        float mBeta = 0.2f;
        void PostSolve() override;
    protected:
        RigidBody* mBodyA;
        RigidBody* mBodyB;
        float mLowerLimit = -1.0f;
        float mUpperLimit = 1.0f;
        bool mLimitEnabled = false;
        bool mLockRotation = false;
    };

}