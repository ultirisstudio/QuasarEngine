#include "qepch.h"
#include "BoxColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Core/Logger.h>

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

        Entity entity{ entt_entity, registry };
        if (!entity.IsValid() || !entity.HasComponent<RigidBodyComponent>()) return;

        auto& rb = entity.GetComponent<RigidBodyComponent>();
        physx::PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        scene->lockWrite();

        if (m_Shape)
        {
            if (m_AttachedActor)
                m_AttachedActor->detachShape(*m_Shape);
            m_Shape->release();
            m_Shape = nullptr;
            m_AttachedActor = nullptr;
        }

        if (!m_Material)
        {
            m_Material = sdk->createMaterial(friction, friction, bounciness);
            m_OwnsMaterial = true;
        }

        const glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        const physx::PxVec3    he = ToPx(full * 1.0f);

        physx::PxShape* shape = sdk->createShape(physx::PxBoxGeometry(he), *m_Material, true);
        if (shape)
        {
            shape->setContactOffset(0.02f);
            shape->setRestOffset(0.005f);
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
            shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, false);

            shape->acquireReference();

            actor->attachShape(*shape);

            m_Shape = shape;
            m_AttachedActor = actor;
        }

        scene->unlockWrite();

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;

        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::UpdateColliderSize()
    {
        Entity entity{ entt_entity, registry };
        if (!m_Shape) { AttachOrRebuild(); return; }

        const glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
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

        const glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        const float volume = std::max(0.000001f, full.x * full.y * full.z);
        const float densityLocal = std::max(0.000001f, mass / volume);

        physx::PxRigidBodyExt::updateMassAndInertia(*dyn, densityLocal);
        dyn->setMass(mass);
    }

    void BoxColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
    {
        if (!m_Shape) return;

        if (auto* scene = PhysicEngine::Instance().GetScene())
            scene->lockWrite();

        actor.detachShape(*m_Shape);
        m_Shape->release();
        m_Shape = nullptr;
        m_AttachedActor = nullptr;

        if (auto* scene = PhysicEngine::Instance().GetScene())
            scene->unlockWrite();
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

        if (m_Shape)
        {
            if (m_AttachedActor && scene)
            {
                scene->lockWrite();
                m_AttachedActor->detachShape(*m_Shape);
                scene->unlockWrite();
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
}