#include "qepch.h"

#include "ConvexMeshColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    ConvexMeshColliderComponent::ConvexMeshColliderComponent() {}
    ConvexMeshColliderComponent::~ConvexMeshColliderComponent()
    {
        if (m_Shape) { m_Shape->release(); m_Shape = nullptr; }
        if (m_Convex) { m_Convex->release(); m_Convex = nullptr; }
        if (m_Material) { m_Material->release(); m_Material = nullptr; }
    }

    void ConvexMeshColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void ConvexMeshColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor || m_Points.empty()) return;

        if (!m_Dirty && m_Shape) return;

        if (m_Shape) { actor->detachShape(*m_Shape); m_Shape->release(); m_Shape = nullptr; }
        if (m_Convex) { m_Convex->release(); m_Convex = nullptr; }

        std::vector<PxVec3> pts; pts.reserve(m_Points.size());
        glm::vec3 scale(1.f);
        if (m_UseEntityScale) scale = entity.GetComponent<TransformComponent>().Scale;
        for (auto& p : m_Points) pts.emplace_back(p.x * scale.x, p.y * scale.y, p.z * scale.z);

        PxConvexMeshDesc desc;
        desc.points.count = static_cast<uint32_t>(pts.size());
        desc.points.stride = sizeof(PxVec3);
        desc.points.data = pts.data();
        desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

        m_Convex = PxCreateConvexMesh(phys.GetPhysics()->getTolerancesScale(), desc);
        if (!m_Convex) return;

        PxConvexMeshGeometry geom(m_Convex);
        m_Shape = sdk->createShape(geom, *m_Material, true);
        if (!m_Shape) return;

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        actor->attachShape(*m_Shape);
        m_Dirty = false;
        RecomputeMass();
    }

    void ConvexMeshColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void ConvexMeshColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void ConvexMeshColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void ConvexMeshColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
    {
        if (!m_Shape) return;

        if (auto* scene = PhysicEngine::Instance().GetScene()) {
            PxWriteLockGuard lock(scene);
            actor.detachShape(*m_Shape);
        }
        else {
            actor.detachShape(*m_Shape);
        }

        m_Shape->release();
        m_Shape = nullptr;
    }

    void ConvexMeshColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;

        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);

        RecomputeMass();
    }

    void ConvexMeshColliderComponent::UpdateColliderSize()
    {
        m_Dirty = true;
        AttachOrRebuild();
    }

    void ConvexMeshColliderComponent::RecomputeMass()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !m_Shape) return;

        const float densityVal = std::max(0.000001f, mass);
        PxRigidBodyExt::updateMassAndInertia(*dyn, densityVal);
        dyn->setMass(mass);
    }

    void ConvexMeshColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        SetFilterDataOnShape(*m_Shape, layer, mask);
    }
}