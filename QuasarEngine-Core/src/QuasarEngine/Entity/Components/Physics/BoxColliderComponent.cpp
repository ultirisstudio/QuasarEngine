#include "qepch.h"
#include "BoxColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Core/Logger.h>

#include <QuasarEngine/Physic/PhysXQueryUtils.h>

namespace QuasarEngine
{
    BoxColliderComponent::BoxColliderComponent() {}

    BoxColliderComponent::~BoxColliderComponent()
    {
        Destroy();
    }

    BoxColliderComponent::BoxColliderComponent(BoxColliderComponent&& other) noexcept
        : PrimitiveColliderComponent(std::move(other))
    {
        m_Shape = other.m_Shape;         other.m_Shape = nullptr;
        m_Material = other.m_Material;      other.m_Material = nullptr;
        m_AttachedActor = other.m_AttachedActor; other.m_AttachedActor = nullptr;

        m_Destroyed = other.m_Destroyed;     other.m_Destroyed = true;
        m_OwnsMaterial = other.m_OwnsMaterial;  other.m_OwnsMaterial = false;

        m_UseEntityScale = other.m_UseEntityScale;
        m_Size = other.m_Size;

        m_IsTrigger = other.m_IsTrigger;
        m_LocalPosition = other.m_LocalPosition;
        m_LocalRotation = other.m_LocalRotation;
        m_FrictionCombine = other.m_FrictionCombine;
        m_RestitutionCombine = other.m_RestitutionCombine;

        friction = other.friction;
        bounciness = other.bounciness;
        mass = other.mass;
    }

    BoxColliderComponent& BoxColliderComponent::operator=(BoxColliderComponent&& other) noexcept
    {
        if (this == &other) return *this;

        Destroy();

        m_Shape = other.m_Shape;         other.m_Shape = nullptr;
        m_Material = other.m_Material;      other.m_Material = nullptr;
        m_AttachedActor = other.m_AttachedActor; other.m_AttachedActor = nullptr;

        m_Destroyed = other.m_Destroyed;     other.m_Destroyed = true;
        m_OwnsMaterial = other.m_OwnsMaterial;  other.m_OwnsMaterial = false;

        m_UseEntityScale = other.m_UseEntityScale;
        m_Size = other.m_Size;

        m_IsTrigger = other.m_IsTrigger;
        m_LocalPosition = other.m_LocalPosition;
        m_LocalRotation = other.m_LocalRotation;
        m_FrictionCombine = other.m_FrictionCombine;
        m_RestitutionCombine = other.m_RestitutionCombine;

        friction = other.friction;
        bounciness = other.bounciness;
        mass = other.mass;

        return *this;
    }

    BoxColliderComponent::BoxColliderComponent(const BoxColliderComponent& other)
        : PrimitiveColliderComponent(other)
    {
        m_UseEntityScale = other.m_UseEntityScale;
        m_Size = other.m_Size;

        m_IsTrigger = other.m_IsTrigger;
        m_LocalPosition = other.m_LocalPosition;
        m_LocalRotation = other.m_LocalRotation;
        m_FrictionCombine = other.m_FrictionCombine;
        m_RestitutionCombine = other.m_RestitutionCombine;

        friction = other.friction;
        bounciness = other.bounciness;
        mass = other.mass;

        m_Shape = nullptr;
        m_Material = nullptr;
        m_AttachedActor = nullptr;
        m_OwnsMaterial = false;
        m_Destroyed = false;
    }

    BoxColliderComponent& BoxColliderComponent::operator=(const BoxColliderComponent& other)
    {
        if (this == &other) return *this;

        Destroy();

        m_UseEntityScale = other.m_UseEntityScale;
        m_Size = other.m_Size;

        m_IsTrigger = other.m_IsTrigger;
        m_LocalPosition = other.m_LocalPosition;
        m_LocalRotation = other.m_LocalRotation;
        m_FrictionCombine = other.m_FrictionCombine;
        m_RestitutionCombine = other.m_RestitutionCombine;

        friction = other.friction;
        bounciness = other.bounciness;
        mass = other.mass;

        m_Shape = nullptr;
        m_Material = nullptr;
        m_AttachedActor = nullptr;
        m_OwnsMaterial = false;
        m_Destroyed = false;

        return *this;
    }

    void BoxColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        if (!m_Material)
        {
            m_Material = sdk->createMaterial(friction, friction, bounciness);
            m_OwnsMaterial = true;
        }

        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void BoxColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxPhysics* sdk = phys.GetPhysics();
        physx::PxScene* scene = phys.GetScene();
        if (!sdk || !scene) return;

        if (m_Shape)
        {
            if (m_AttachedActor)
                m_AttachedActor->detachShape(*m_Shape);
            m_Shape->release();
            m_Shape = nullptr;
            m_AttachedActor = nullptr;
        }

        Entity entity{ entt_entity, registry };
        if (!entity.IsValid() || !entity.HasComponent<RigidBodyComponent>()) return;

        auto& rb = entity.GetComponent<RigidBodyComponent>();
        m_AttachedActor = rb.GetActor();
        if (!m_AttachedActor) return;

        //scene->lockWrite();
        PxWriteLockGuard lock(scene);

        if (!m_Material)
        {
            m_Material = sdk->createMaterial(friction, friction, bounciness);
            m_OwnsMaterial = true;
        }

        glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        full = glm::max(glm::abs(full), glm::vec3(0.0001f));
        const physx::PxVec3 he = ToPx(full * 1.0f);

        m_Shape = sdk->createShape(physx::PxBoxGeometry(he), *m_Material, true);
        if (!m_Shape) return;
        
        m_Shape->setContactOffset(0.02f);
        m_Shape->setRestOffset(0.005f);
        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);

        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));

        SetFilterDataOnShape(*m_Shape, 0xFFFFFFFFu, 0xFFFFFFFFu);

        //m_Shape->acquireReference();

        m_AttachedActor->attachShape(*m_Shape);

        //scene->unlockWrite();

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void BoxColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void BoxColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void BoxColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;

        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::UpdateColliderSize()
    {
        Entity entity{ entt_entity, registry };
        if (!m_Shape) { AttachOrRebuild(); return; }

        glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        full = glm::max(glm::abs(full), glm::vec3(0.0001f));
        const physx::PxBoxGeometry geom(ToPx(full * 1.0f));

        if (auto* scene = PhysicEngine::Instance().GetScene())
        {
            scene->lockWrite();
            m_Shape->setGeometry(geom);
            scene->unlockWrite();
        }
        else
        {
            m_Shape->setGeometry(geom);
        }

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::RecomputeMassFromSize()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.IsValid() || !entity.HasComponent<RigidBodyComponent>()) return;

        auto& rb = entity.GetComponent<RigidBodyComponent>();
        physx::PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !m_Shape) return;

        const float targetMass = std::max(0.0001f, mass);

        if (auto* scene = PhysicEngine::Instance().GetScene())
        {
            PxWriteLockGuard lock(scene);

            physx::PxRigidBodyExt::updateMassAndInertia(*dyn, 1.0f);
            const float baseMass = dyn->getMass();
            if (baseMass > 1e-6f)
            {
                const float s = targetMass / baseMass;
                dyn->setMass(targetMass);
                dyn->setMassSpaceInertiaTensor(dyn->getMassSpaceInertiaTensor() * s);
            }
            return;
        }

        physx::PxRigidBodyExt::updateMassAndInertia(*dyn, 1.0f);
        const float baseMass = dyn->getMass();
        if (baseMass > 1e-6f)
        {
            const float s = targetMass / baseMass;
            dyn->setMass(targetMass);
            dyn->setMassSpaceInertiaTensor(dyn->getMassSpaceInertiaTensor() * s);
        }
    }

    void BoxColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
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
        m_AttachedActor = nullptr;
    }

    void BoxColliderComponent::Destroy()
    {
        if (m_Destroyed) return;
        m_Destroyed = true;

        if (!PhysicEngine::Instance().GetPhysics())
        {
            m_Shape = nullptr;
            m_Material = nullptr;
            m_AttachedActor = nullptr;
            m_OwnsMaterial = false;
            return;
        }

        physx::PxScene* scene = PhysicEngine::Instance().GetScene();

        if (m_Shape) {
            if (m_AttachedActor && scene) {
                PxWriteLockGuard lock(scene);
                m_AttachedActor->detachShape(*m_Shape);
            }
            m_Shape->release();
            m_Shape = nullptr;
            m_AttachedActor = nullptr;
        }

        if (m_Material)
        {
            if (m_OwnsMaterial)
            {
                m_Material->release();
            }
            m_Material = nullptr;
            m_OwnsMaterial = false;
        }
    }

    void BoxColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
		SetFilterDataOnShape(*m_Shape, layer, mask);
    }
}