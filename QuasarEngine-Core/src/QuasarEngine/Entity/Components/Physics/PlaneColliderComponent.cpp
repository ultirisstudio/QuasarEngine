#include "qepch.h"

#include "PlaneColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <glm/gtc/quaternion.hpp>

using namespace physx;

namespace QuasarEngine
{
    static inline PxVec3 ToPx(const glm::vec3& v) { return { v.x, v.y, v.z }; }
    static inline PxQuat ToPx(const glm::quat& q) { return { q.x, q.y, q.z, q.w }; }

    PlaneColliderComponent::PlaneColliderComponent() {}
    PlaneColliderComponent::~PlaneColliderComponent()
    {
        //if (mShape) { mShape->release();    mShape = nullptr; }
        //if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void PlaneColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    PxTransform PlaneColliderComponent::MakeLocalPose() const
    {
        if (m_UseEntityOrientation)
        {
            Entity entity{ entt_entity, registry };
            const glm::quat q = entity.HasComponent<TransformComponent>()
                ? glm::quat(entity.GetComponent<TransformComponent>().Rotation)
                : glm::quat(1, 0, 0, 0);
            const PxQuat rot = ToPx(q);
            const PxVec3 off(0.f, m_Distance, 0.f);
            return PxTransform(off, rot);
        }
        else
        {
            glm::vec3 n = m_Normal;
            const float len = std::max(1e-6f, std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z));
            n /= len;
            const PxPlane plane(ToPx(n), m_Distance);
            return PxTransformFromPlaneEquation(plane);
        }
    }

    void PlaneColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        PxScene* scene = phys.GetScene();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();

        PxRigidActor* base = rb.GetActor();
        PxRigidStatic* actor = base ? base->is<PxRigidStatic>() : nullptr;
        if (!actor) return;

        const bool useLock = (scene != nullptr);
        if (useLock) scene->lockWrite();

        if (mShape) { actor->detachShape(*mShape); mShape->release(); mShape = nullptr; }

        PxShape* shape = sdk->createShape(PxPlaneGeometry(), *mMaterial, true);
        if (shape)
        {
            shape->setLocalPose(MakeLocalPose());
            actor->attachShape(*shape);
            mShape = shape;
        }

        if (useLock) scene->unlockWrite();
    }

    void PlaneColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
    }

    void PlaneColliderComponent::UpdateColliderSize()
    {
        auto& phys = PhysicEngine::Instance();
        PxScene* scene = phys.GetScene();

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidStatic* actor = rb.GetActor() ? rb.GetActor()->is<PxRigidStatic>() : nullptr;
        if (!actor) return;

        if (!mShape) { AttachOrRebuild(); return; }

        if (scene) scene->lockWrite();
        mShape->setLocalPose(MakeLocalPose());
        if (scene) scene->unlockWrite();
    }
}