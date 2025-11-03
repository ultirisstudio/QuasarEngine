#include "qepch.h"

#include "TriangleMeshColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    TriangleMeshColliderComponent::TriangleMeshColliderComponent() {}
    TriangleMeshColliderComponent::~TriangleMeshColliderComponent()
    {
        if (m_Shape) { m_Shape->release(); m_Shape = nullptr; }
        if (m_TriMesh) { m_TriMesh->release(); m_TriMesh = nullptr; }
        if (m_Material) { m_Material->release(); m_Material = nullptr; }
    }

    void TriangleMeshColliderComponent::SetMesh(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices)
    {
        m_Vertices = verts;
        m_Indices = indices;
        mDirty = true;
    }

    void TriangleMeshColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void TriangleMeshColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk || m_Vertices.empty() || m_Indices.empty()) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        if (rb.GetDynamic()) return;

        if (!mDirty && m_Shape) return;

        if (m_Shape) { actor->detachShape(*m_Shape); m_Shape->release(); m_Shape = nullptr; }
        if (m_TriMesh) { m_TriMesh->release(); m_TriMesh = nullptr; }

        std::vector<PxVec3> pts; pts.reserve(m_Vertices.size());
        glm::vec3 scale(1.f);
        if (m_UseEntityScale) scale = entity.GetComponent<TransformComponent>().Scale;
        for (auto& v : m_Vertices) pts.emplace_back(v.x * scale.x, v.y * scale.y, v.z * scale.z);

        PxTriangleMeshDesc desc;
        desc.points.count = static_cast<uint32_t>(pts.size());
        desc.points.stride = sizeof(PxVec3);
        desc.points.data = pts.data();
        desc.triangles.count = static_cast<uint32_t>(m_Indices.size() / 3);
        desc.triangles.stride = 3 * sizeof(uint32_t);
        desc.triangles.data = m_Indices.data();
        desc.flags = PxMeshFlag::e16_BIT_INDICES;

        std::vector<uint16_t> idx16;

        if (m_Indices.size() < (1u << 16))
        {
            idx16.resize(m_Indices.size());
            for (size_t i = 0; i < m_Indices.size(); ++i)
                idx16[i] = static_cast<uint16_t>(m_Indices[i]);

            desc.flags = PxMeshFlag::e16_BIT_INDICES;
            desc.triangles.data = idx16.data();
            desc.triangles.stride = 3 * sizeof(uint16_t);
        }
        else
        {
            desc.triangles.data = m_Indices.data();
            desc.triangles.stride = 3 * sizeof(uint32_t);
        }

        m_TriMesh = PxCreateTriangleMesh(phys.GetPhysics()->getTolerancesScale(), desc);
        if (!m_TriMesh) return;

        PxTriangleMeshGeometry geom(m_TriMesh);
        m_Shape = sdk->createShape(geom, *m_Material, true);
        if (!m_Shape) return;

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        actor->attachShape(*m_Shape);
        mDirty = false;
    }

    void TriangleMeshColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void TriangleMeshColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void TriangleMeshColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void TriangleMeshColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;
        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);
    }

    void TriangleMeshColliderComponent::UpdateColliderSize()
    {
        mDirty = true;
        AttachOrRebuild();
    }

    void TriangleMeshColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        physx::PxFilterData qfd = m_Shape->getQueryFilterData();
        qfd.word0 = layer;
        qfd.word1 = mask;
        m_Shape->setQueryFilterData(qfd);
    }
}