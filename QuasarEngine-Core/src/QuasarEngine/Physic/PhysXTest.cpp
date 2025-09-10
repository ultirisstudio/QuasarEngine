#include "qepch.h"

#include "PhysXTest.h"
#include <iostream>

namespace QuasarEngine
{
    using namespace physx;
    PhysXTest::PhysXTest()
    {
        /*mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
        mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true);

        PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        mDispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = mDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;

        mScene = mPhysics->createScene(sceneDesc);

        mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

        // Ground plane
        PxTransform groundPose(PxVec3(0.f, -1.f, 0.f));
        PxPlane base(PxVec3(0.f, 1.f, 0.f), 0.f);
        PxPlane plane = base.transform(groundPose);
        mGround = PxCreatePlane(*mPhysics, plane, *mMaterial);
        mScene->addActor(*mGround);

        // Dynamic box
        PxTransform boxPose(PxVec3(0.0f, 10.0f, 0.0f));
        mBox = PxCreateDynamic(*mPhysics, boxPose, PxBoxGeometry(0.5f, 0.5f, 0.5f), *mMaterial, 10.0f);
        mScene->addActor(*mBox);

        std::cout << "PhysXTest initialized: box at height 10" << std::endl;*/
    }

    PhysXTest::~PhysXTest()
    {
        /*if (mScene)      mScene->release();
        if (mDispatcher) mDispatcher->release();
        if (mPhysics)    mPhysics->release();
        if (mFoundation) mFoundation->release();*/
    }

    void PhysXTest::StepSimulation(float dt)
    {
        /*mScene->simulate(dt);
        mScene->fetchResults(true);

        PxTransform pose = mBox->getGlobalPose();
        std::cout << "Box height: " << pose.p.y << std::endl;*/
    }
}