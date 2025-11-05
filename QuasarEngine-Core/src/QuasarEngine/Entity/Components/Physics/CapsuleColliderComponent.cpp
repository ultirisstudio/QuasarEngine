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

        PxQuat axisRot = PxQuat(PxIdentity);
        if (m_Axis == Axis::Y)      axisRot = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
        else if (m_Axis == Axis::Z) axisRot = PxQuat(-PxHalfPi, PxVec3(0, 1, 0));
        const PxTransform userLocal(PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z), PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w));
        m_Shape->setLocalPose(PxTransform(userLocal.p, axisRot * userLocal.q));

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);

        SetFilterDataOnShape(*m_Shape, 0xFFFFFFFFu, 0xFFFFFFFFu);

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
            PxQuat axisRot = PxQuat(PxIdentity);
            if (m_Axis == Axis::Y)      axisRot = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
            else if (m_Axis == Axis::Z) axisRot = PxQuat(-PxHalfPi, PxVec3(0, 1, 0));
            const PxTransform userLocal(PxVec3(p.x, p.y, p.z), PxQuat(r.x, r.y, r.z, r.w));
            m_Shape->setLocalPose(PxTransform(userLocal.p, axisRot * userLocal.q));
        }
    }

    void CapsuleColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void CapsuleColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
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
        SetFilterDataOnShape(*m_Shape, layer, mask);
    }
}