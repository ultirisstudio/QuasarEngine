#pragma once

#include <PxPhysicsAPI.h>

#define QE_ENABLE_PVD 0

#if QE_ENABLE_PVD
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>
#endif

#include <entt.hpp>
#include <sol/sol.hpp>

#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>

#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>

#include <QuasarEngine/Physic/PhysXQueryUtils.h>

namespace QuasarEngine
{
    class PxLoggerCallback final : public physx::PxErrorCallback
    {
    public:
        void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
    };

    class PhysicEngine
    {
    public:
        static PhysicEngine& Instance();

        PhysicEngine(const PhysicEngine&) = delete;
        PhysicEngine& operator=(const PhysicEngine&) = delete;
        PhysicEngine(PhysicEngine&&) = delete;
        PhysicEngine& operator=(PhysicEngine&&) = delete;

        bool Initialize(uint32_t numThreads = 2, const physx::PxVec3& gravity = physx::PxVec3(0.f, -9.81f, 0.f), bool enableCCD = true, bool enablePVD = true, const char* pvdHost = "127.0.0.1", uint32_t pvdPort = 5425);
        void Shutdown();

        void Step(double dt, double fixedTimestep = 1.f / 60.f, uint32_t maxSubsteps = 4);

        physx::PxPhysics* GetPhysics() const noexcept { return m_Physics; }
        physx::PxScene* GetScene() const noexcept { return m_Scene; }
        physx::PxMaterial* GetDefaultMaterial() const noexcept { return m_DefaultMaterial; }
        physx::PxFoundation* GetFoundation() const noexcept { return m_Foundation; }
        physx::PxCpuDispatcher* GetDispatcher() const noexcept { return m_Dispatcher; }

        physx::PxControllerManager* GetCCTManager() const { return m_CCTManager; }
        physx::PxMaterial* GetCCTMaterial();
        bool HasScene() const { return m_Scene != nullptr; }

        physx::PxMaterial* CreateMaterial(float sf = 0.5f, float df = 0.5f, float r = 0.1f);
        void AddActor(physx::PxActor& actor);
        void RemoveActor(physx::PxActor& actor);
        void SetGravity(const physx::PxVec3& g);
        physx::PxVec3 GetGravity() const;
        
        bool RaycastAll(const physx::PxVec3& origin, const physx::PxVec3& dir, float maxDist, std::vector<physx::PxRaycastHit>& outHits, const QueryOptions& opts);
        bool Raycast(const physx::PxVec3& origin, const physx::PxVec3& dir, float maxDist, physx::PxRaycastHit& outHit, const QueryOptions& opts);

        bool SweepBox(const physx::PxVec3& halfExtents, const physx::PxTransform& pose, const physx::PxVec3& dir, float maxDist, physx::PxSweepHit& outHit, const QueryOptions& opts);
        bool SweepCapsule(float radius, float halfHeight, const physx::PxTransform& pose, const physx::PxVec3& dir, float maxDist, physx::PxSweepHit& outHit, const QueryOptions& opts);

        bool OverlapBox(const physx::PxVec3& halfExtents, const physx::PxTransform& pose, std::vector<physx::PxOverlapHit>& outHits, const QueryOptions& opts);
        bool OverlapCapsule(float radius, float halfHeight, const physx::PxTransform& pose, std::vector<physx::PxOverlapHit>& outHits, const QueryOptions& opts);

        std::shared_ptr<VertexArray> GetDebugVertexArray() const { return m_VertexArray; }
        uint32_t GetDebugVertexCount() const { return m_DebugVertexCount; }

        void SetVisualizationScale(float scale);
        void EnableVisualization(bool shapes, bool aabbs, bool axes);

        void RegisterActor(physx::PxActor* a, entt::entity id);
        void UnregisterActor(physx::PxActor* a);

        sol::object ActorToEntityObject(physx::PxActor* a, sol::state_view lua);

    private:
        PhysicEngine() = default;
        ~PhysicEngine();

        void ReleaseAll();
        void BuildOrUpdateDebugBuffer(const std::vector<float>& vertices);

        physx::PxDefaultAllocator m_Allocator;
        PxLoggerCallback m_ErrorCallback;

        physx::PxFoundation* m_Foundation = nullptr;
        physx::PxPhysics* m_Physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_Dispatcher = nullptr;
        physx::PxScene* m_Scene = nullptr;
        physx::PxMaterial* m_DefaultMaterial = nullptr;

#if QE_ENABLE_PVD
        physx::PxPvd* m_Pvd = nullptr;
        physx::PxPvdTransport* m_PvdTransport = nullptr;
        bool m_PvdConnected = false;
#endif

        physx::PxCookingParams m_CookingParams{ physx::PxTolerancesScale() };

        float m_Accumulator = 0.f;
        bool m_Initialized = false;

        std::unordered_map<const physx::PxActor*, entt::entity> m_ActorToEntity;

        physx::PxControllerManager* m_CCTManager = nullptr;
        mutable physx::PxMaterial* m_CCTMaterial = nullptr;

        std::shared_ptr<VertexArray> m_VertexArray;
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        uint32_t m_DebugVertexCount = 0;
    };
}
