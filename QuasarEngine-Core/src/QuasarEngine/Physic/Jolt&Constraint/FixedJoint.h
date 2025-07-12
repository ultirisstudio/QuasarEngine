#pragma once

#include "Joint.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtc/matrix_transform.hpp>

namespace QuasarEngine {

    class FixedJoint : public Joint {
    public:
        FixedJoint(RigidBody* A, RigidBody* B);
        FixedJoint(RigidBody* A, RigidBody* B, const glm::vec3& anchor);

        void PreSolve(float dt) override;
        void SolveConstraint() override;
        void PostSolve() override {}

        JointType GetType() const override { return JointType::Fixed; }

        void SetLocalAnchorA(const glm::vec3& anchor) { mLocalAnchorA = anchor; }

        float mAngularStiffness = 10.0f;

    private:
        glm::vec3 mLocalAnchorA;
        glm::vec3 mLocalAnchorB;

        glm::quat mInitialRelativeOrientation;

        glm::vec3 mBiasTranslation;
        glm::vec3 mAccumulatedImpulseTranslation;

        float mBeta = 0.2f;

    protected:
        RigidBody* mBodyA;
        RigidBody* mBodyB;
    };

}