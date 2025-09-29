#pragma once

#include <PxPhysicsAPI.h>

#define QE_ENABLE_PVD 0

#if QE_ENABLE_PVD
#  include <pvd/PxPvd.h>
#  include <pvd/PxPvdTransport.h>
#endif

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// #define QE_COORDS_Z_UP

namespace QuasarEngine
{
#ifndef QE_COORDS_Z_UP
    inline physx::PxVec3  ToPx(const glm::vec3& v) noexcept { return { v.x, v.y, v.z }; }
    inline glm::vec3      ToGlm(const physx::PxVec3& v) noexcept { return { v.x, v.y, v.z }; }

    inline physx::PxQuat  ToPx(const glm::quat& q) noexcept { return { q.x, q.y, q.z, q.w }; }
    inline glm::quat      ToGlm(const physx::PxQuat& q) noexcept { return { q.w, q.x, q.y, q.z }; }

    inline physx::PxTransform ToPx(const glm::vec3& p, const glm::quat& r) noexcept { return { ToPx(p), ToPx(r) }; }
    inline void              ToGlm(const physx::PxTransform& t, glm::vec3& outPos, glm::quat& outRot) noexcept {
        outPos = ToGlm(t.p);
        outRot = ToGlm(t.q);
    }
#else
    inline physx::PxVec3  ToPx(const glm::vec3& v) noexcept { return { v.x, v.z, v.y }; }
    inline glm::vec3      ToGlm(const physx::PxVec3& v) noexcept { return { v.x, v.z, v.y }; }

    inline physx::PxQuat  ToPx(const glm::quat& q) noexcept { return { q.x, q.z, q.y, q.w }; }
    inline glm::quat      ToGlm(const physx::PxQuat& q) noexcept { return { q.w, q.x, q.z, q.y }; }

    inline physx::PxTransform ToPx(const glm::vec3& p, const glm::quat& r) noexcept { return { ToPx(p), ToPx(r) }; }
    inline void              ToGlm(const physx::PxTransform& t, glm::vec3& outPos, glm::quat& outRot) noexcept {
        outPos = ToGlm(t.p);
        outRot = ToGlm(t.q);
    }
#endif

    class PxLoggerCallback final : public physx::PxErrorCallback
    {
    public:
        void reportError(physx::PxErrorCode::Enum code,
            const char* message,
            const char* file,
            int line) override;
    };

    class PhysicEngine
    {
    public:
        static PhysicEngine& Instance();

        PhysicEngine(const PhysicEngine&) = delete;
        PhysicEngine& operator=(const PhysicEngine&) = delete;
        PhysicEngine(PhysicEngine&&) = delete;
        PhysicEngine& operator=(PhysicEngine&&) = delete;

        bool Initialize(uint32_t numThreads = 2,
            const physx::PxVec3& gravity = physx::PxVec3(0.f, -9.81f, 0.f),
            bool enableCCD = true,
            bool enablePVD = true,
            const char* pvdHost = "127.0.0.1",
            uint32_t pvdPort = 5425);

        void Shutdown();

        void Step(float dt, float fixedTimestep = 1.f / 60.f, uint32_t maxSubsteps = 4);

        physx::PxPhysics* GetPhysics()        const noexcept { return mPhysics; }
        physx::PxScene* GetScene()          const noexcept { return mScene; }
        physx::PxMaterial* GetDefaultMaterial()const noexcept { return mDefaultMaterial; }
        physx::PxFoundation* GetFoundation()     const noexcept { return mFoundation; }
        physx::PxCpuDispatcher* GetDispatcher()     const noexcept { return mDispatcher; }

        physx::PxMaterial* CreateMaterial(float sf = 0.5f, float df = 0.5f, float r = 0.1f);
        physx::PxRigidStatic* CreateStaticPlane(const physx::PxVec3& n, float distance, physx::PxMaterial* mat = nullptr);
        physx::PxRigidStatic* CreateStaticBox(const physx::PxTransform& pose, const physx::PxVec3& halfExtents, physx::PxMaterial* mat = nullptr);
        physx::PxRigidDynamic* CreateDynamicBox(const physx::PxTransform& pose, const physx::PxVec3& halfExtents, float density = 10.f, physx::PxMaterial* mat = nullptr);
        physx::PxRigidDynamic* CreateDynamicSphere(const physx::PxTransform& pose, float radius, float density = 10.f, physx::PxMaterial* mat = nullptr);

        void AddActor(physx::PxActor& actor);
        void RemoveActor(physx::PxActor& actor);

        void SetGravity(const physx::PxVec3& g);
        physx::PxVec3 GetGravity() const;

        bool Raycast(const physx::PxVec3& origin,
            const physx::PxVec3& unitDir,
            float maxDistance,
            physx::PxRaycastBuffer& outHit) const;

        physx::PxConvexMesh* CreateConvexMesh(const physx::PxConvexMeshDesc& desc);
        physx::PxTriangleMesh* CreateTriangleMesh(const physx::PxTriangleMeshDesc& desc);

    private:
        PhysicEngine() = default;
        ~PhysicEngine();

        void ReleaseAll();

        physx::PxDefaultAllocator         mAllocator;
        PxLoggerCallback                  mErrorCallback;

        physx::PxFoundation* mFoundation = nullptr;
        physx::PxPhysics* mPhysics = nullptr;
        physx::PxDefaultCpuDispatcher* mDispatcher = nullptr;
        physx::PxScene* mScene = nullptr;
        physx::PxMaterial* mDefaultMaterial = nullptr;

        physx::PxCookingParams   mCookingParams{ physx::PxTolerancesScale() };

#if QE_ENABLE_PVD
        physx::PxPvd* mPvd = nullptr;
        physx::PxPvdTransport* mPvdTransport = nullptr;
        bool                              mPvdConnected = false;
#endif

        float mAccumulator = 0.f;
        bool  mInitialized = false;
    };
}
