#include "qepch.h"

#include "BoxColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    BoxColliderComponent::BoxColliderComponent() {}

    BoxColliderComponent::~BoxColliderComponent()
    {
        //if (mShape) { mShape->release(); mShape = nullptr; }
        //if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void BoxColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void BoxColliderComponent::AttachOrRebuild()
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
        glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        PxVec3 he = ToPx(full * 0.5f);

        PxShape* shape = sdk->createShape(PxBoxGeometry(he), *mMaterial, true);
        if (!shape) return;
        shape->setContactOffset(0.02f);
        shape->setRestOffset(0.005f);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);

        if (old) actor->detachShape(*old);
        if (old) old->release();
        mShape = shape;

        actor->attachShape(*mShape);

        RecomputeMassFromSize();
    }

    void BoxColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
        RecomputeMassFromSize();
    }

    void BoxColliderComponent::UpdateColliderSize()
    {
        AttachOrRebuild();
    }

    void BoxColliderComponent::RecomputeMassFromSize()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !mShape) return;

        glm::vec3 full = m_UseEntityScale ? entity.GetComponent<TransformComponent>().Scale : m_Size;
        const float volume = std::max(0.000001f, full.x * full.y * full.z);
        const float density = std::max(0.000001f, mass / volume);
        PxRigidBodyExt::updateMassAndInertia(*dyn, density);
        dyn->setMass(mass);
    }
}