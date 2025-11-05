#include "qepch.h"

#include "PlaneColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <glm/gtc/quaternion.hpp>

#include <QuasarEngine/Physic/PhysXQueryUtils.h>

namespace QuasarEngine
{
    PlaneColliderComponent::PlaneColliderComponent() {}
    PlaneColliderComponent::~PlaneColliderComponent()
    {
        if (m_Shape) { m_Shape->release();    m_Shape = nullptr; }
        if (m_Material) { m_Material->release(); m_Material = nullptr; }
    }

    void PlaneColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    physx::PxTransform PlaneColliderComponent::MakeLocalPose() const
    {
        if (m_UseEntityOrientation)
        {
            Entity entity{ entt_entity, registry };
            const glm::quat q = entity.HasComponent<TransformComponent>()
                ? glm::quat(entity.GetComponent<TransformComponent>().Rotation)
                : glm::quat(1, 0, 0, 0);
            const physx::PxQuat rot = ToPx(q);
            const physx::PxVec3 off(0.f, m_Distance, 0.f);
            return physx::PxTransform(off, rot);
        }
        else
        {
            glm::vec3 n = m_Normal;
            const float len = std::max(1e-6f, std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z));
            n /= len;
            const physx::PxPlane plane(ToPx(n), m_Distance);
            return physx::PxTransformFromPlaneEquation(plane);
        }
    }

    void PlaneColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxPhysics* sdk = phys.GetPhysics();
        physx::PxScene* scene = phys.GetScene();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();

        physx::PxRigidActor* base = rb.GetActor();
        physx::PxRigidStatic* actor = base ? base->is<physx::PxRigidStatic>() : nullptr;
        if (!actor) return;

        const bool useLock = (scene != nullptr);
        if (useLock) scene->lockWrite();

        if (m_Shape) { actor->detachShape(*m_Shape); m_Shape->release(); m_Shape = nullptr; }

        m_Shape = sdk->createShape(physx::PxPlaneGeometry(), *m_Material, true);
        if (!m_Shape) return;
        
        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        m_Shape->setLocalPose(MakeLocalPose());
        actor->attachShape(*m_Shape);

        if (useLock) scene->unlockWrite();
    }

    void PlaneColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void PlaneColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void PlaneColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void PlaneColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
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

    void PlaneColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;
        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);
    }

    void PlaneColliderComponent::UpdateColliderSize()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxScene* scene = phys.GetScene();

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        physx::PxRigidStatic* actor = rb.GetActor() ? rb.GetActor()->is<physx::PxRigidStatic>() : nullptr;
        if (!actor) return;

        if (!m_Shape) { AttachOrRebuild(); return; }

        if (scene) scene->lockWrite();
        m_Shape->setLocalPose(MakeLocalPose());
        if (scene) scene->unlockWrite();
    }

    void PlaneColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        SetFilterDataOnShape(*m_Shape, layer, mask);
    }
}