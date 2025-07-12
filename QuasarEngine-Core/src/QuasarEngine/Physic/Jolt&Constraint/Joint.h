#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine {

    class RigidBody;

    enum class JointType {
        Distance,
        Hinge,
        Fixed,
        Slider
    };

    class Joint {
    public:
        Joint(RigidBody* a, RigidBody* b)
            : bodyA(a), bodyB(b) {
        }

        virtual ~Joint() = default;

        virtual JointType GetType() const = 0;

        // Appel� une fois par frame avant SolveConstraint
        virtual void PreSolve(float dt) = 0;

        // Appel� plusieurs fois par frame
        virtual void SolveConstraint() = 0;

        virtual void PostSolve() = 0;

        RigidBody* GetBodyA() const { return bodyA; }
        RigidBody* GetBodyB() const { return bodyB; }

    protected:
        RigidBody* bodyA;
        RigidBody* bodyB;
    };

}