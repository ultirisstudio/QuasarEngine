#pragma once

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class PhysXTest
    {
    public:
        PhysXTest();
        ~PhysXTest();

        void StepSimulation(float dt);

    private:
        physx::PxDefaultAllocator      mAllocator;
        physx::PxDefaultErrorCallback  mErrorCallback;

        physx::PxFoundation* mFoundation = nullptr;
        physx::PxPhysics* mPhysics = nullptr;
        physx::PxDefaultCpuDispatcher* mDispatcher = nullptr;
        physx::PxScene* mScene = nullptr;
        physx::PxMaterial* mMaterial = nullptr;

        physx::PxRigidStatic* mGround = nullptr;
        physx::PxRigidDynamic* mBox = nullptr;
    };
}