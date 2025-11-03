#include "qepch.h"

#include "CapsuleColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    static inline float Max3(float a, float b, float c) { return std::max(a, std::max(b, c)); }

    CapsuleColliderComponent::CapsuleColliderComponent() {}
    CapsuleColliderComponent::~CapsuleColliderComponent()
    {
        if (m_Shape) { m_Shape->release(); m_Shape = nullptr; }
        if (m_Material) { m_Material->release(); m_Material = nullptr; }
    }

    void CapsuleColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void CapsuleColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        if (m_Shape) { actor->detachShape(*m_Shape); m_Shape->release(); }

        float scaleRadius = 1.f, scaleHeight = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            switch (m_Axis)
            {
            case Axis::X: scaleHeight = std::abs(s.x); scaleRadius = Max3(1e-4f, std::abs(s.y), std::abs(s.z)); break;
            case Axis::Y: scaleHeight = std::abs(s.y); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.z)); break;
            case Axis::Z: scaleHeight = std::abs(s.z); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.y)); break;
            }
        }

        const float radius = std::max(0.0001f, m_Radius * scaleRadius);
        const float halfHeight = std::max(0.0001f, 0.5f * m_Height * scaleHeight);

        PxCapsuleGeometry geom(radius, halfHeight);

        m_Shape = sdk->createShape(geom, *m_Material, true);
        if (!m_Shape) return;

        PxQuat rot = PxQuat(PxIdentity);
        if (m_Axis == Axis::Y)      rot = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
        else if (m_Axis == Axis::Z) rot = PxQuat(-PxHalfPi, PxVec3(0, 1, 0));
        m_Shape->setLocalPose(PxTransform(PxVec3(0), rot));

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        actor->attachShape(*m_Shape);

        RecomputeMassFromSize();
    }

    void CapsuleColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void CapsuleColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void CapsuleColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void CapsuleColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;

        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);

        RecomputeMassFromSize();
    }

    void CapsuleColliderComponent::UpdateColliderSize()
    {
        AttachOrRebuild();
    }

    void CapsuleColliderComponent::RecomputeMassFromSize()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !m_Shape) return;

        float scaleRadius = 1.f, scaleHeight = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            switch (m_Axis)
            {
            case Axis::X: scaleHeight = std::abs(s.x); scaleRadius = Max3(1e-4f, std::abs(s.y), std::abs(s.z)); break;
            case Axis::Y: scaleHeight = std::abs(s.y); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.z)); break;
            case Axis::Z: scaleHeight = std::abs(s.z); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.y)); break;
            }
        }

        const float r = std::max(0.0001f, m_Radius * scaleRadius);
        const float hh = std::max(0.0001f, 0.5f * m_Height * scaleHeight);
        const float cylinderHeight = 2.0f * hh;
        const float volume = (3.14159265358979323846f * r * r * cylinderHeight) + (4.0f / 3.0f) * 3.14159265358979323846f * r * r * r;
        const float densityVal = std::max(0.000001f, mass / volume);

        PxRigidBodyExt::updateMassAndInertia(*dyn, densityVal);
        dyn->setMass(mass);
    }

    void CapsuleColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        physx::PxFilterData qfd = m_Shape->getQueryFilterData();
        qfd.word0 = layer;
        qfd.word1 = mask;
        m_Shape->setQueryFilterData(qfd);
    }
}