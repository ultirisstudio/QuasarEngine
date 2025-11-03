#include "qepch.h"

#include "SphereColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    SphereColliderComponent::SphereColliderComponent() {}
    SphereColliderComponent::~SphereColliderComponent()
    {
        if (m_Shape) { m_Shape->release(); m_Shape = nullptr; }
        if (m_Material) { m_Material->release(); m_Material = nullptr; }
    }

    void SphereColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void SphereColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        auto* sdk = phys.GetPhysics();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        PxShape* old = m_Shape;

        float scale = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            scale = std::max(s.x, std::max(s.y, s.z));
        }
        const float radius = std::max(0.0001f, m_Radius * scale);

        PxShape* shape = sdk->createShape(PxSphereGeometry(radius), *m_Material, true);
        if (!shape) return;

        if (old) actor->detachShape(*old);
        if (old) old->release();
        m_Shape = shape;

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        actor->attachShape(*m_Shape);

        RecomputeMassFromSize();
    }

    void SphereColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void SphereColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void SphereColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void SphereColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;

        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);

        RecomputeMassFromSize();
    }

    void SphereColliderComponent::UpdateColliderSize()
    {
        AttachOrRebuild();
    }

    void SphereColliderComponent::RecomputeMassFromSize()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !m_Shape) return;

        float scale = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            scale = std::max(s.x, std::max(s.y, s.z));
        }
        const float r = std::max(0.0001f, m_Radius * scale);
        const float volume = (4.0f / 3.0f) * 3.14159265358979323846f * r * r * r;
        const float densityVal = std::max(0.000001f, mass / volume);

        PxRigidBodyExt::updateMassAndInertia(*dyn, densityVal);
        dyn->setMass(mass);
    }

    void SphereColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        physx::PxFilterData qfd = m_Shape->getQueryFilterData();
        qfd.word0 = layer;
        qfd.word1 = mask;
        m_Shape->setQueryFilterData(qfd);
    }
}
