#include "qepch.h"

#include "PhysicEngine.h"
#include "PhysXQueryUtils.h"

#include <iostream>
#include <algorithm>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
    void PxLoggerCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        const char* level = "INFO";
        switch (code)
        {
        case physx::PxErrorCode::eABORT: level = "ABORT"; break;
        case physx::PxErrorCode::eINVALID_PARAMETER:
        case physx::PxErrorCode::eINVALID_OPERATION:
        case physx::PxErrorCode::eOUT_OF_MEMORY:
        case physx::PxErrorCode::eINTERNAL_ERROR:
        case physx::PxErrorCode::ePERF_WARNING:
        case physx::PxErrorCode::eDEBUG_WARNING: level = "WARN"; break;
        case physx::PxErrorCode::eDEBUG_INFO: level = "INFO"; break;
        default: level = "LOG"; break;
        }
        std::cerr << "[PhysX][" << level << "] " << message << " (" << file << ":" << line << ")" << std::endl;
    }

    PhysicEngine& PhysicEngine::Instance()
    {
        static PhysicEngine s_Instance;
        return s_Instance;
    }

    bool PhysicEngine::Initialize(uint32_t numThreads, const physx::PxVec3& gravity, bool enableCCD, bool enablePVD, const char* pvdHost, uint32_t pvdPort)
    {
        if (m_Initialized) return true;
        m_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
        if (!m_Foundation) return false;

#if QE_ENABLE_PVD
        PxPvd* pvd = nullptr;
        if (enablePVD)
        {
            m_Pvd = PxCreatePvd(*m_Foundation);
            if (m_Pvd)
            {
                m_PvdTransport = PxDefaultPvdSocketTransportCreate(pvdHost, pvdPort, 10);
                if (m_PvdTransport) m_PvdConnected = m_Pvd->connect(*m_PvdTransport, PxPvdInstrumentationFlag::eALL);
                pvd = m_Pvd;
            }
        }
        m_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, PxTolerancesScale(), true, pvd);
#else
        (void)enablePVD; (void)pvdHost; (void)pvdPort;
        m_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, physx::PxTolerancesScale(), true);
#endif
        if (!m_Physics) { ReleaseAll(); return false; }

#if QE_ENABLE_PVD
        PxInitExtensions(*m_Physics, m_Pvd);
#else
        PxInitExtensions(*m_Physics, nullptr);
#endif

        physx::PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
        sceneDesc.gravity = gravity;
        m_Dispatcher = physx::PxDefaultCpuDispatcherCreate(std::max<uint32_t>(1, numThreads));
        if (!m_Dispatcher) { ReleaseAll(); return false; }
        sceneDesc.cpuDispatcher = m_Dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        if (enableCCD) sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
        sceneDesc.solverType = physx::PxSolverType::eTGS;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_STABILIZATION;
        sceneDesc.flags |= physx::PxSceneFlag::eREQUIRE_RW_LOCK;
        sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eSAP;

        m_Scene = m_Physics->createScene(sceneDesc);
        if (!m_Scene) { ReleaseAll(); return false; }

#if QE_ENABLE_PVD
        if (m_Pvd && m_PvdConnected)
        {
            if (PxPvdSceneClient* client = m_Scene->getScenePvdClient())
            {
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
            }
        }
#endif

        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f);
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, 1.0f);
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);

        m_CCTManager = PxCreateControllerManager(*m_Scene);

        m_DefaultMaterial = CreateMaterial(0.5f, 0.5f, 0.1f);
        if (!m_DefaultMaterial) { ReleaseAll(); return false; }

        m_CookingParams = physx::PxCookingParams(m_Physics->getTolerancesScale());
        m_CookingParams.suppressTriangleMeshRemapTable = true;

        m_Initialized = true;
        return true;
    }

    void PhysicEngine::Shutdown()
    {
        if (!m_Initialized) return;
        ReleaseAll();
        m_Initialized = false;
    }

    PhysicEngine::~PhysicEngine()
    {
        ReleaseAll();
    }

    void PhysicEngine::ReleaseAll()
    {
        m_VertexBuffer.reset();
        m_VertexArray.reset();
        if (m_CCTManager) { m_CCTManager->purgeControllers(); m_CCTManager->release(); m_CCTManager = nullptr; }
        if (m_CCTMaterial) { m_CCTMaterial->release(); m_CCTMaterial = nullptr; }
        if (m_Scene) { m_Scene->release(); m_Scene = nullptr; }
        if (m_Dispatcher) { m_Dispatcher->release(); m_Dispatcher = nullptr; }
        if (m_DefaultMaterial) { m_DefaultMaterial->release(); m_DefaultMaterial = nullptr; }
        if (m_Physics) { m_Physics->release(); m_Physics = nullptr; }
#if QE_ENABLE_PVD
        if (m_Pvd)
        {
            if (m_PvdConnected) m_Pvd->disconnect();
            m_PvdConnected = false;
        }
        if (m_PvdTransport) { m_PvdTransport->release(); m_PvdTransport = nullptr; }
        if (m_Pvd) { m_Pvd->release(); m_Pvd = nullptr; }
#endif
        if (m_Foundation) { m_Foundation->release(); m_Foundation = nullptr; }
        m_Accumulator = 0.f;
        m_DebugVertexCount = 0;
    }

    void PhysicEngine::BuildOrUpdateDebugBuffer(const std::vector<float>& vertices)
    {
        m_DebugVertexCount = static_cast<uint32_t>(vertices.size() / 6);
        if (!m_VertexArray) m_VertexArray = VertexArray::Create();
        if (!m_VertexBuffer)
        {
            m_VertexBuffer = VertexBuffer::Create(static_cast<uint32_t>(vertices.size() * sizeof(float)));
            m_VertexBuffer->SetLayout({ { ShaderDataType::Vec3, "inPosition" }, { ShaderDataType::Vec3, "inColor" } });
            m_VertexArray->AddVertexBuffer(m_VertexBuffer);
        }
        m_VertexBuffer->Upload(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));
    }

    void PhysicEngine::Step(double dt, double fixedTimestep, uint32_t maxSubsteps)
    {
        if (!m_Scene) return;

        if (dt < 0.0) dt = 0.0;

        m_Accumulator += static_cast<float>(dt);

        const float maxAccum = static_cast<float>(fixedTimestep * maxSubsteps);
        if (m_Accumulator > maxAccum)
            m_Accumulator = maxAccum;

        uint32_t substeps = 0;
        while (m_Accumulator >= fixedTimestep && substeps < maxSubsteps)
        {
            m_Scene->simulate(static_cast<float>(fixedTimestep));
            m_Scene->fetchResults(true);
            m_Accumulator -= static_cast<float>(fixedTimestep);
            ++substeps;
        }

        /*std::vector<float> vertices;
        vertices.reserve(m_DebugVertexCount ? m_DebugVertexCount * 6 : 4096);
        const physx::PxRenderBuffer& rb = m_Scene->getRenderBuffer();
        for (physx::PxU32 i = 0; i < rb.getNbLines(); ++i)
        {
            const physx::PxDebugLine& l = rb.getLines()[i];
            physx::PxU32 c0 = l.color0;
            float r0 = ((c0 >> 16) & 0xFF) / 255.0f;
            float g0 = ((c0 >> 8) & 0xFF) / 255.0f;
            float b0 = (c0 & 0xFF) / 255.0f;
            vertices.push_back(l.pos0.x); vertices.push_back(l.pos0.y); vertices.push_back(l.pos0.z);
            vertices.push_back(r0); vertices.push_back(g0); vertices.push_back(b0);

            physx::PxU32 c1 = l.color1;
            float r1 = ((c1 >> 16) & 0xFF) / 255.0f;
            float g1 = ((c1 >> 8) & 0xFF) / 255.0f;
            float b1 = (c1 & 0xFF) / 255.0f;
            vertices.push_back(l.pos1.x); vertices.push_back(l.pos1.y); vertices.push_back(l.pos1.z);
            vertices.push_back(r1); vertices.push_back(g1); vertices.push_back(b1);
        }
        BuildOrUpdateDebugBuffer(vertices);*/
    }

    physx::PxMaterial* PhysicEngine::GetCCTMaterial()
    {
        if (!m_CCTMaterial) {
            m_CCTMaterial = m_Physics->createMaterial(0.0f, 0.0f, 0.0f);
        }
        return m_CCTMaterial;
    }


    physx::PxMaterial* PhysicEngine::CreateMaterial(float sf, float df, float r)
    {
        return m_Physics ? m_Physics->createMaterial(sf, df, r) : nullptr;
    }

    void PhysicEngine::AddActor(physx::PxActor& actor)
    {
        if (!m_Scene) return;
        m_Scene->lockWrite();
        m_Scene->addActor(actor);
        m_Scene->unlockWrite();
    }

    void PhysicEngine::RemoveActor(physx::PxActor& actor)
    {
        if (!m_Scene) return;
        m_Scene->lockWrite();
        m_Scene->removeActor(actor);
        m_Scene->unlockWrite();
    }

    void PhysicEngine::SetGravity(const physx::PxVec3& g)
    {
        if (m_Scene) m_Scene->setGravity(g);
    }

    physx::PxVec3 PhysicEngine::GetGravity() const
    {
        return m_Scene ? m_Scene->getGravity() : physx::PxVec3(0.f);
    }

    bool PhysicEngine::RaycastAll(const physx::PxVec3& origin, const physx::PxVec3& dir, float maxDist, std::vector<physx::PxRaycastHit>& outHits, const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;

        physx::PxHitFlags flags = opts.hitFlags;
        if (opts.bothSides) flags |= physx::PxHitFlag::eMESH_BOTH_SIDES;

        QueryFilterCB cb; cb.includeTriggers = opts.includeTriggers;
        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        const uint32_t maxHits = 256;
        physx::PxRaycastHit hits[maxHits];
        physx::PxRaycastBuffer hitBuf(hits, maxHits);

        if (dir.magnitudeSquared() <= 1e-12f) return false;
        const bool ok = scene->raycast(origin, dir.getNormalized(), maxDist, hitBuf, flags, qfd, &cb);
        if (!ok) return false;

        outHits.clear();
        outHits.reserve(hitBuf.getNbAnyHits());
        
        for (uint32_t i = 0; i < hitBuf.getNbTouches(); ++i) outHits.push_back(hitBuf.getTouches()[i]);
        if (hitBuf.hasBlock) outHits.push_back(hitBuf.block);
        return !outHits.empty();
    }

    bool PhysicEngine::Raycast(const physx::PxVec3& origin, const physx::PxVec3& dir, float maxDist, physx::PxRaycastHit& outHit, const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;
        if (dir.magnitudeSquared() <= 1e-12f) return false;

        physx::PxRaycastBuffer hit;
        physx::PxHitFlags flags = opts.hitFlags;
        if (opts.bothSides) flags |= physx::PxHitFlag::eMESH_BOTH_SIDES;

        QueryFilterCB cb;
        cb.includeTriggers = opts.includeTriggers;

        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        if (!scene->raycast(origin, dir.getNormalized(), maxDist, hit, flags, qfd, &cb))
            return false;

        if (!hit.hasBlock) return false;
        outHit = hit.block;
        return true;
    }

    bool PhysicEngine::SweepBox(const physx::PxVec3& halfExtents, const physx::PxTransform& pose,
        const physx::PxVec3& dir, float maxDist, physx::PxSweepHit& outHit,
        const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;

        physx::PxBoxGeometry geom(halfExtents);
        physx::PxSweepBuffer hit;
        physx::PxHitFlags flags = opts.hitFlags;
        if (opts.preciseSweep) flags |= physx::PxHitFlag::ePRECISE_SWEEP;

        QueryFilterCB cb;
        cb.includeTriggers = opts.includeTriggers;

        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        if (dir.magnitudeSquared() <= 1e-12f) return false;
        if (!scene->sweep(geom, pose, dir.getNormalized(), maxDist, hit, flags, qfd, &cb))
            return false;

        if (!hit.hasBlock) return false;
        outHit = hit.block;
        return true;
    }

    bool PhysicEngine::SweepCapsule(float radius, float halfHeight, const physx::PxTransform& pose, const physx::PxVec3& dir, float maxDist, physx::PxSweepHit& outHit, const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;

        physx::PxCapsuleGeometry geom(radius, halfHeight);
        physx::PxSweepBuffer hit;
        physx::PxHitFlags flags = opts.hitFlags;
        if (opts.preciseSweep) flags |= physx::PxHitFlag::ePRECISE_SWEEP;

        QueryFilterCB cb;
        cb.includeTriggers = opts.includeTriggers;

        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        if (dir.magnitudeSquared() <= 1e-12f) return false;
        if (!scene->sweep(geom, pose, dir.getNormalized(), maxDist, hit, flags, qfd, &cb))
            return false;

        if (!hit.hasBlock) return false;
        outHit = hit.block;
        return true;
    }

    bool PhysicEngine::OverlapBox(const physx::PxVec3& halfExtents, const physx::PxTransform& pose, std::vector<physx::PxOverlapHit>& outHits, const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;

        physx::PxBoxGeometry geom(halfExtents);
        const uint32_t maxHits = 256;
        physx::PxOverlapHit hits[maxHits];
        physx::PxOverlapBuffer buf(hits, maxHits);

        QueryFilterCB cb;
        cb.includeTriggers = opts.includeTriggers;

        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        const bool ok = scene->overlap(geom, pose, buf, qfd, &cb);
        if (!ok) return false;

        outHits.assign(buf.getTouches(), buf.getTouches() + buf.getNbTouches());
        return !outHits.empty();
    }

    bool PhysicEngine::OverlapCapsule(float radius, float halfHeight, const physx::PxTransform& pose, std::vector<physx::PxOverlapHit>& outHits, const QueryOptions& opts)
    {
        physx::PxScene* scene = m_Scene;
        if (!scene) return false;

        physx::PxCapsuleGeometry geom(radius, halfHeight);
        const uint32_t maxHits = 256;
        physx::PxOverlapHit hits[maxHits];
        physx::PxOverlapBuffer buf(hits, maxHits);

        QueryFilterCB cb;
        cb.includeTriggers = opts.includeTriggers;

        physx::PxQueryFilterData qfd = MakeFilterData(opts);

        const bool ok = scene->overlap(geom, pose, buf, qfd, &cb);
        if (!ok) return false;

        outHits.assign(buf.getTouches(), buf.getTouches() + buf.getNbTouches());
        return !outHits.empty();
    }


    void PhysicEngine::SetVisualizationScale(float scale)
    {
        if (m_Scene) m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, scale);
    }

    void PhysicEngine::EnableVisualization(bool shapes, bool aabbs, bool axes)
    {
        if (!m_Scene) return;
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, shapes ? 1.0f : 0.0f);
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_AABBS, aabbs ? 1.0f : 0.0f);
        m_Scene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, axes ? 1.0f : 0.0f);
    }

    void PhysicEngine::RegisterActor(physx::PxActor* a, entt::entity id) {
        if (!a) return;
        a->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
        m_ActorToEntity[a] = id;
    }

    void PhysicEngine::UnregisterActor(physx::PxActor* a) {
        if (!a) return;
        a->userData = nullptr;
        m_ActorToEntity.erase(a);
    }

    sol::object PhysicEngine::ActorToEntityObject(physx::PxActor* a, sol::state_view lua) {
        if (!a) return sol::nil;
        if (a->userData) {
            const auto id = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(a->userData));
            auto* reg = Renderer::Instance().m_SceneData.m_Scene->GetRegistry();
            Entity e{ id, reg };
            return e.IsValid() ? sol::make_object(lua, e) : sol::nil;
        }
        auto it = m_ActorToEntity.find(a);
        if (it != m_ActorToEntity.end()) {
            auto* reg = Renderer::Instance().m_SceneData.m_Scene->GetRegistry();
            Entity e{ it->second, reg };
            return e.IsValid() ? sol::make_object(lua, e) : sol::nil;
        }
        return sol::nil;
    }
}
