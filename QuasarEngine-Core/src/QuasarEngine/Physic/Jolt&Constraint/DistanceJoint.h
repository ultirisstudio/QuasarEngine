#pragma once

#include "Joint.h"
#include <glm/glm.hpp>

namespace QuasarEngine {

    class DistanceJoint : public Joint {
    public:
        DistanceJoint(RigidBody* bodyA, RigidBody* bodyB,
            const glm::vec3& anchorA, const glm::vec3& anchorB);

        virtual JointType GetType() const override { return JointType::Distance; }

        virtual void PreSolve(float dt) override;
        virtual void SolveConstraint() override;
        void PostSolve() override {}

    private:
        glm::vec3 localAnchorA;
        glm::vec3 localAnchorB;

        glm::vec3 worldAnchorA;
        glm::vec3 worldAnchorB;

        float targetDistance;
    };

}