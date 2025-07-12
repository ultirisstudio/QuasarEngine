#pragma once

#include "Joint.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine {

    class BallAndSocketJoint : public Joint {
    public:
        BallAndSocketJoint(RigidBody* bodyA, RigidBody* bodyB, const glm::vec3& anchor);

        void PreSolve(float dt) override;
        void SolveConstraint() override;

    private:
        glm::vec3 mLocalAnchorA;
        glm::vec3 mLocalAnchorB;

        glm::vec3 mWorldAnchorA;
        glm::vec3 mWorldAnchorB;

        glm::vec3 mBiasTranslation;
        glm::vec3 mAccumulatedImpulseTranslation = glm::vec3(0.0f);

        float mBeta = 0.2f;

    protected:
        RigidBody* mBodyA;
        RigidBody* mBodyB;
    };

}