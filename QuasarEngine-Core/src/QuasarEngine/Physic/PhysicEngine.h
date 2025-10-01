#pragma once

#include <PxPhysicsAPI.h>

#define QE_ENABLE_PVD 0

#if QE_ENABLE_PVD
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>
#endif

#include <memory>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
#ifndef QE_COORDS_Z_UP
    inline physx::PxVec3 ToPx(const glm::vec3& v) noexcept { return { v.x, v.y, v.z }; }
    inline glm::vec3 ToGlm(const physx::PxVec3& v) noexcept { return { v.x, v.y, v.z }; }
    inline physx::PxQuat ToPx(const glm::quat& q) noexcept { return { q.x, q.y, q.z, q.w }; }
    inline glm::quat ToGlm(const physx::PxQuat& q) noexcept { return { q.w, q.x, q.y, q.z }; }
    inline physx::PxTransform ToPx(const glm::vec3& p, const glm::quat& r) noexcept { return { ToPx(p), ToPx(r) }; }
    inline void ToGlm(const physx::PxTransform& t, glm::vec3& outPos, glm::quat& outRot) noexcept { outPos = ToGlm(t.p); outRot = ToGlm(t.q); }
#else
    inline physx::PxVec3 ToPx(const glm::vec3& v) noexcept { return { v.x, v.z, v.y }; }
    inline glm::vec3 ToGlm(const physx::PxVec3& v) noexcept { return { v.x, v.z, v.y }; }
    inline physx::PxQuat ToPx(const glm::quat& q) noexcept { return { q.x, q.z, q.y, q.w }; }
    inline glm::quat ToGlm(const physx::PxQuat& q) noexcept { return { q.w, q.x, q.z, q.y }; }
    inline physx::PxTransform ToPx(const glm::vec3& p, const glm::quat& r) noexcept { return { ToPx(p), ToPx(r) }; }
    inline void ToGlm(const physx::PxTransform& t, glm::vec3& outPos, glm::quat& outRot) noexcept { outPos = ToGlm(t.p); outRot = ToGlm(t.q); }
#endif

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
        void Step(float dt, float fixedTimestep = 1.f / 60.f, uint32_t maxSubsteps = 4);

        physx::PxPhysics* GetPhysics() const noexcept { return m_Physics; }
        physx::PxScene* GetScene() const noexcept { return m_Scene; }
        physx::PxMaterial* GetDefaultMaterial() const noexcept { return m_DefaultMaterial; }
        physx::PxFoundation* GetFoundation() const noexcept { return m_Foundation; }
        physx::PxCpuDispatcher* GetDispatcher() const noexcept { return m_Dispatcher; }

        physx::PxMaterial* CreateMaterial(float sf = 0.5f, float df = 0.5f, float r = 0.1f);
        void AddActor(physx::PxActor& actor);
        void RemoveActor(physx::PxActor& actor);
        void SetGravity(const physx::PxVec3& g);
        physx::PxVec3 GetGravity() const;
        bool Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, float maxDistance, physx::PxRaycastBuffer& outHit) const;

        std::shared_ptr<VertexArray> GetDebugVertexArray() const { return m_VertexArray; }
        uint32_t GetDebugVertexCount() const { return m_DebugVertexCount; }
        void SetVisualizationScale(float scale);
        void EnableVisualization(bool shapes, bool aabbs, bool axes);

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

        std::shared_ptr<VertexArray> m_VertexArray;
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        uint32_t m_DebugVertexCount = 0;
    };
}
