#pragma once

#include "Joint.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine {

    class HingeJoint : public Joint {
    public:
        HingeJoint(RigidBody* bodyA, RigidBody* bodyB, const glm::vec3& anchor, const glm::vec3& axis);

        void PreSolve(float dt) override;
        void SolveConstraint() override;

        void PostSolve() override {}
        JointType GetType() const override { return JointType::Hinge; }
    private:
        glm::vec3 mLocalAnchorA;
        glm::vec3 mLocalAnchorB;
        glm::vec3 mLocalAxisA;
        glm::vec3 mLocalAxisB;

        glm::vec3 mWorldAnchorA;
        glm::vec3 mWorldAnchorB;
        glm::vec3 mWorldAxis;

        glm::vec3 mBiasTranslation;
        glm::vec3 mAccumulatedImpulseTranslation = glm::vec3(0.0f);

        float mBeta = 0.2f;
        float mAngularStiffness = 0.3f; // raideur de rotation autour de l’axe hinge
        float mMinAngle = -glm::half_pi<float>(); // -90°
        float mMaxAngle = glm::half_pi<float>();  // +90°
        bool mEnableLimits = true;

    protected:
        RigidBody* mBodyA;
        RigidBody* mBodyB;
    };

}