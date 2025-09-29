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
        if (mShape) { mShape->release(); mShape = nullptr; }
        if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void SphereColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
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

        PxShape* old = mShape;

        float scale = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            scale = std::max(s.x, std::max(s.y, s.z));
        }
        const float radius = std::max(0.0001f, m_Radius * scale);

        PxShape* shape = sdk->createShape(PxSphereGeometry(radius), *mMaterial, true);
        if (!shape) return;

        if (old) actor->detachShape(*old);
        if (old) old->release();
        mShape = shape;
        actor->attachShape(*mShape);

        RecomputeMassFromSize();
    }

    void SphereColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
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
        if (!dyn || !mShape) return;

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
}
