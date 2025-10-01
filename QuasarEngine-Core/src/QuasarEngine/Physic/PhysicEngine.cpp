#include "qepch.h"
#include "PhysicEngine.h"
#include <iostream>
#include <algorithm>

using namespace physx;

namespace QuasarEngine
{
    void PxLoggerCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        const char* level = "INFO";
        switch (code)
        {
        case PxErrorCode::eABORT: level = "ABORT"; break;
        case PxErrorCode::eINVALID_PARAMETER:
        case PxErrorCode::eINVALID_OPERATION:
        case PxErrorCode::eOUT_OF_MEMORY:
        case PxErrorCode::eINTERNAL_ERROR:
        case PxErrorCode::ePERF_WARNING:
        case PxErrorCode::eDEBUG_WARNING: level = "WARN"; break;
        case PxErrorCode::eDEBUG_INFO: level = "INFO"; break;
        default: level = "LOG"; break;
        }
        std::cerr << "[PhysX][" << level << "] " << message << " (" << file << ":" << line << ")" << std::endl;
    }

    PhysicEngine& PhysicEngine::Instance()
    {
        static PhysicEngine s_Instance;
        return s_Instance;
    }

    bool PhysicEngine::Initialize(uint32_t numThreads, const PxVec3& gravity, bool enableCCD, bool enablePVD, const char* pvdHost, uint32_t pvdPort)
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
        m_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, PxTolerancesScale(), true);
#endif
        if (!m_Physics) { ReleaseAll(); return false; }

#if QE_ENABLE_PVD
        PxInitExtensions(*m_Physics, m_Pvd);
#else
        PxInitExtensions(*m_Physics, nullptr);
#endif

        PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
        sceneDesc.gravity = gravity;
        m_Dispatcher = PxDefaultCpuDispatcherCreate(std::max<uint32_t>(1, numThreads));
        if (!m_Dispatcher) { ReleaseAll(); return false; }
        sceneDesc.cpuDispatcher = m_Dispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        if (enableCCD) sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        sceneDesc.solverType = PxSolverType::eTGS;
        sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
        sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;

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

        m_Scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);

        m_DefaultMaterial = CreateMaterial(0.5f, 0.5f, 0.1f);
        if (!m_DefaultMaterial) { ReleaseAll(); return false; }

        m_CookingParams = PxCookingParams(m_Physics->getTolerancesScale());
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

    void PhysicEngine::Step(float dt, float fixedTimestep, uint32_t maxSubsteps)
    {
        if (!m_Scene) return;
        m_Accumulator += dt;
        uint32_t substeps = 0;
        while (m_Accumulator >= fixedTimestep && substeps < maxSubsteps)
        {
            m_Scene->simulate(fixedTimestep);
            m_Scene->fetchResults(true);
            m_Accumulator -= fixedTimestep;
            ++substeps;
        }
        if (substeps == 0 && dt > 0.f)
        {
            m_Scene->simulate(dt);
            m_Scene->fetchResults(true);
        }

        std::vector<float> vertices;
        vertices.reserve(m_DebugVertexCount ? m_DebugVertexCount * 6 : 4096);
        const PxRenderBuffer& rb = m_Scene->getRenderBuffer();
        for (PxU32 i = 0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& l = rb.getLines()[i];
            PxU32 c0 = l.color0;
            float r0 = ((c0 >> 16) & 0xFF) / 255.0f;
            float g0 = ((c0 >> 8) & 0xFF) / 255.0f;
            float b0 = (c0 & 0xFF) / 255.0f;
            vertices.push_back(l.pos0.x); vertices.push_back(l.pos0.y); vertices.push_back(l.pos0.z);
            vertices.push_back(r0); vertices.push_back(g0); vertices.push_back(b0);

            PxU32 c1 = l.color1;
            float r1 = ((c1 >> 16) & 0xFF) / 255.0f;
            float g1 = ((c1 >> 8) & 0xFF) / 255.0f;
            float b1 = (c1 & 0xFF) / 255.0f;
            vertices.push_back(l.pos1.x); vertices.push_back(l.pos1.y); vertices.push_back(l.pos1.z);
            vertices.push_back(r1); vertices.push_back(g1); vertices.push_back(b1);
        }
        BuildOrUpdateDebugBuffer(vertices);
    }

    PxMaterial* PhysicEngine::CreateMaterial(float sf, float df, float r)
    {
        return m_Physics ? m_Physics->createMaterial(sf, df, r) : nullptr;
    }

    void PhysicEngine::AddActor(PxActor& actor)
    {
        if (m_Scene) m_Scene->addActor(actor);
    }

    void PhysicEngine::RemoveActor(PxActor& actor)
    {
        if (m_Scene) m_Scene->removeActor(actor);
    }

    void PhysicEngine::SetGravity(const PxVec3& g)
    {
        if (m_Scene) m_Scene->setGravity(g);
    }

    PxVec3 PhysicEngine::GetGravity() const
    {
        return m_Scene ? m_Scene->getGravity() : PxVec3(0.f);
    }

    bool PhysicEngine::Raycast(const PxVec3& origin, const PxVec3& unitDir, float maxDistance, PxRaycastBuffer& outHit) const
    {
        if (!m_Scene) return false;
        return m_Scene->raycast(origin, unitDir, maxDistance, outHit);
    }

    void PhysicEngine::SetVisualizationScale(float scale)
    {
        if (m_Scene) m_Scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, scale);
    }

    void PhysicEngine::EnableVisualization(bool shapes, bool aabbs, bool axes)
    {
        if (!m_Scene) return;
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, shapes ? 1.0f : 0.0f);
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, aabbs ? 1.0f : 0.0f);
        m_Scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, axes ? 1.0f : 0.0f);
    }
}
