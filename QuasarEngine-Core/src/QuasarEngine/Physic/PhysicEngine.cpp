#include "qepch.h"

#include "PhysicEngine.h"

#include <iostream>
#include <cassert>

using namespace physx;

namespace QuasarEngine
{
    void PxLoggerCallback::reportError(PxErrorCode::Enum code,
        const char* message,
        const char* file,
        int line)
    {
        const char* level = "INFO";
        switch (code)
        {
        case PxErrorCode::eABORT:        level = "ABORT"; break;
        case PxErrorCode::eINVALID_PARAMETER:
        case PxErrorCode::eINVALID_OPERATION:
        case PxErrorCode::eOUT_OF_MEMORY:
        case PxErrorCode::eINTERNAL_ERROR:
        case PxErrorCode::ePERF_WARNING:
        case PxErrorCode::eDEBUG_WARNING: level = "WARN";  break;
        case PxErrorCode::eDEBUG_INFO:    level = "INFO";  break;
        default:                          level = "LOG";   break;
        }
        std::cerr << "[PhysX][" << level << "] " << message
            << " (" << file << ":" << line << ")"
            << std::endl;
    }

    PhysicEngine& PhysicEngine::Instance()
    {
        static PhysicEngine s_instance;
        return s_instance;
    }

    bool PhysicEngine::Initialize(uint32_t numThreads,
        const PxVec3& gravity,
        bool enableCCD,
        bool enablePVD,
        const char* pvdHost,
        uint32_t pvdPort)
    {
        if (mInitialized) return true;

        mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
        if (!mFoundation)
        {
            std::cerr << "[PhysX] PxCreateFoundation failed." << std::endl;
            return false;
        }

#if QE_ENABLE_PVD
        std::cout << "[PVD] Build WITH PVD support (QE_ENABLE_PVD=1)\n";
        PxPvd* pvd = nullptr;
        if (enablePVD) {
            mPvd = PxCreatePvd(*mFoundation);
            if (!mPvd) {
                std::cerr << "[PVD] PxCreatePvd returned nullptr. Your PhysX build may have PVD disabled.\n";
            }
            else {
                mPvdTransport = PxDefaultPvdSocketTransportCreate(pvdHost, pvdPort, 10);
                if (!mPvdTransport) {
                    std::cerr << "[PVD] PxDefaultPvdSocketTransportCreate failed.\n";
                }
                else {
                    mPvdConnected = mPvd->connect(*mPvdTransport, PxPvdInstrumentationFlag::eALL);
                    std::cout << "[PVD] connect(" << pvdHost << ":" << pvdPort << ") = "
                        << (mPvdConnected ? "true" : "false") << "\n";
                }
                pvd = mPvd;
            }
        }
        mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, pvd);
#else
        (void)enablePVD; (void)pvdHost; (void)pvdPort;
        std::cout << "[PVD] Build WITHOUT PVD support (QE_ENABLE_PVD=0)\n";
        mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true);
#endif
        if (!mPhysics)
        {
            std::cerr << "[PhysX] PxCreatePhysics failed." << std::endl;
            ReleaseAll();
            return false;
        }

#if QE_ENABLE_PVD
        if (!PxInitExtensions(*mPhysics, mPvd)) {
            std::cerr << "[PhysX] PxInitExtensions failed.\n";
        }
        else {
            std::cout << "[PhysX] PxInitExtensions OK\n";
        }
#endif

        PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
        sceneDesc.gravity = gravity;

        mDispatcher = PxDefaultCpuDispatcherCreate(static_cast<uint32_t>(std::max<uint32_t>(1, numThreads)));
        if (!mDispatcher)
        {
            std::cerr << "[PhysX] PxDefaultCpuDispatcherCreate failed." << std::endl;
            ReleaseAll();
            return false;
        }
        sceneDesc.cpuDispatcher = mDispatcher;

        sceneDesc.filterShader = PxDefaultSimulationFilterShader;

        if (enableCCD)
        {
            sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        }

        sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;

        mScene = mPhysics->createScene(sceneDesc);
        if (!mScene)
        {
            std::cerr << "[PhysX] createScene failed." << std::endl;
            ReleaseAll();
            return false;
        }

        mScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        mScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
        mScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

#if QE_ENABLE_PVD
        if (mPvd && mPvdConnected) {
            if (PxPvdSceneClient* client = mScene->getScenePvdClient()) {
                std::cout << "[PVD] Scene client OK, enabling contact/queries streaming\n";
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
            }
            else {
                std::cerr << "[PVD] Scene client NULL (no streaming). "
                    "Likely PVD not actually connected or PhysX without PVD instrumentation.\n";
            }
        }
        else if (enablePVD) {
            std::cerr << "[PVD] Socket connect failed; enabling file capture pvd_capture.pxd2\n";
            if (mPvd && !mPvdConnected) {
                PxPvdTransport* fileT = PxDefaultPvdFileTransportCreate("pvd_capture.pxd2");
                if (fileT && mPvd->connect(*fileT, PxPvdInstrumentationFlag::eALL)) {
                    std::cout << "[PVD] File capture ON (pvd_capture.pxd2). Open this file in PVD.\n";
                }
                else {
                    std::cerr << "[PVD] File capture connect failed as well.\n";
                }
            }
        }
#endif

        mDefaultMaterial = CreateMaterial(0.5f, 0.5f, 0.1f);
        if (!mDefaultMaterial)
        {
            std::cerr << "[PhysX] Create default material failed." << std::endl;
            ReleaseAll();
            return false;
        }

		mCookingParams = PxCookingParams(mPhysics->getTolerancesScale());
		mCookingParams.suppressTriangleMeshRemapTable = true;

        mInitialized = true;
        std::cout << "[PhysX] Initialized successfully." << std::endl;
        return true;
    }

    void PhysicEngine::Shutdown()
    {
        if (!mInitialized) return;
        ReleaseAll();
        mInitialized = false;
        std::cout << "[PhysX] Shutdown complete." << std::endl;
    }

    PhysicEngine::~PhysicEngine()
    {
        ReleaseAll();
    }

    void PhysicEngine::ReleaseAll()
    {
        if (mScene) { mScene->release();        mScene = nullptr; }
        if (mDispatcher) { mDispatcher->release();   mDispatcher = nullptr; }
        if (mDefaultMaterial) { mDefaultMaterial->release(); mDefaultMaterial = nullptr; }
        if (mPhysics) { mPhysics->release();      mPhysics = nullptr; }

#if QE_ENABLE_PVD
        if (mPvd)
        {
            if (mPvdConnected) mPvd->disconnect();
            mPvdConnected = false;
        }
        if (mPvdTransport) { mPvdTransport->release(); mPvdTransport = nullptr; }
        if (mPvd) { mPvd->release();         mPvd = nullptr; }
#endif

        if (mFoundation) { mFoundation->release();   mFoundation = nullptr; }

        mAccumulator = 0.f;
    }

    void PhysicEngine::Step(float dt, float fixedTimestep, uint32_t maxSubsteps)
    {
        if (!mScene) return;

        mAccumulator += dt;
        uint32_t substeps = 0;

        while (mAccumulator >= fixedTimestep && substeps < maxSubsteps)
        {
            mScene->simulate(fixedTimestep);
            mScene->fetchResults(true);
            mAccumulator -= fixedTimestep;
            ++substeps;
        }

        if (substeps == 0 && dt > 0.f)
        {
            mScene->simulate(dt);
            mScene->fetchResults(true);
        }
    }

    PxMaterial* PhysicEngine::CreateMaterial(float sf, float df, float r)
    {
        return mPhysics ? mPhysics->createMaterial(sf, df, r) : nullptr;
    }

    PxRigidStatic* PhysicEngine::CreateStaticPlane(const PxVec3& n, float distance, PxMaterial* mat)
    {
        if (!mPhysics) return nullptr;
        if (!mat) mat = mDefaultMaterial;
        PxPlane plane(n, -distance);
        return PxCreatePlane(*mPhysics, plane, *mat);
    }

    PxRigidStatic* PhysicEngine::CreateStaticBox(const PxTransform& pose, const PxVec3& halfExtents, PxMaterial* mat)
    {
        if (!mPhysics) return nullptr;
        if (!mat) mat = mDefaultMaterial;
        PxShape* shape = mPhysics->createShape(PxBoxGeometry(halfExtents), *mat, true);
        if (!shape) return nullptr;
        PxRigidStatic* actor = mPhysics->createRigidStatic(pose);
        if (!actor) { shape->release(); return nullptr; }
        actor->attachShape(*shape);
        shape->release();
        return actor;
    }

    PxRigidDynamic* PhysicEngine::CreateDynamicBox(const PxTransform& pose, const PxVec3& halfExtents, float density, PxMaterial* mat)
    {
        if (!mPhysics) return nullptr;
        if (!mat) mat = mDefaultMaterial;
        PxShape* shape = mPhysics->createShape(PxBoxGeometry(halfExtents), *mat, true);
        if (!shape) return nullptr;
        PxRigidDynamic* body = mPhysics->createRigidDynamic(pose);
        if (!body) { shape->release(); return nullptr; }
        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, density);
        shape->release();
        return body;
    }

    PxRigidDynamic* PhysicEngine::CreateDynamicSphere(const PxTransform& pose, float radius, float density, PxMaterial* mat)
    {
        if (!mPhysics) return nullptr;
        if (!mat) mat = mDefaultMaterial;
        PxShape* shape = mPhysics->createShape(PxSphereGeometry(radius), *mat, true);
        if (!shape) return nullptr;
        PxRigidDynamic* body = mPhysics->createRigidDynamic(pose);
        if (!body) { shape->release(); return nullptr; }
        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, density);
        shape->release();
        return body;
    }

    void PhysicEngine::AddActor(PxActor& actor)
    {
        if (mScene) mScene->addActor(actor);
    }

    void PhysicEngine::RemoveActor(PxActor& actor)
    {
        if (mScene) mScene->removeActor(actor);
    }

    void PhysicEngine::SetGravity(const PxVec3& g)
    {
        if (mScene) mScene->setGravity(g);
    }

    PxVec3 PhysicEngine::GetGravity() const
    {
        return mScene ? mScene->getGravity() : PxVec3(0.f);
    }

    bool PhysicEngine::Raycast(const PxVec3& origin,
        const PxVec3& unitDir,
        float maxDistance,
        PxRaycastBuffer& outHit) const
    {
        if (!mScene) return false;
        return mScene->raycast(origin, unitDir, maxDistance, outHit);
    }

    PxConvexMesh* PhysicEngine::CreateConvexMesh(const PxConvexMeshDesc& desc)
    {
        return PxCreateConvexMesh(mCookingParams, desc);
    }

    PxTriangleMesh* PhysicEngine::CreateTriangleMesh(const PxTriangleMeshDesc& desc)
    {
        return PxCreateTriangleMesh(mCookingParams, desc);
    }
}
