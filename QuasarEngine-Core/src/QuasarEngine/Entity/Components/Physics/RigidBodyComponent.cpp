#include "qepch.h"

#include "RigidBodyComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    static inline PxVec3 ToPx(const glm::vec3& v) { return PxVec3(v.x, v.y, v.z); }
    static inline PxQuat ToPx(const glm::quat& q) { return PxQuat(q.x, q.y, q.z, q.w); }
    static inline glm::vec3 ToGlm(const PxVec3& v) { return glm::vec3(v.x, v.y, v.z); }
    static inline glm::quat ToGlm(const PxQuat& q) { return glm::quat(q.w, q.x, q.y, q.z); }

    RigidBodyComponent::RigidBodyComponent() {}
    RigidBodyComponent::~RigidBodyComponent() { Destroy(); }

    void RigidBodyComponent::Destroy()
    {
        if (mActor)
        {
            PhysicEngine::Instance().RemoveActor(*mActor);
            if (mShape) { mShape->release(); mShape = nullptr; }
            if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
            mActor->release();
            mActor = nullptr;
            mDynamic = nullptr;
        }
    }

    RigidBodyComponent::BodyType RigidBodyComponent::ParseBodyType(const std::string& s) const
    {
        if (s == "STATIC") return BodyType::Static;
        if (s == "KINEMATIC") return BodyType::Kinematic;
        return BodyType::Dynamic;
    }

    void RigidBodyComponent::RebuildActor()
    {
        auto& phys = PhysicEngine::Instance();
        auto* sdk = phys.GetPhysics();
        auto* scene = phys.GetScene();
        if (!sdk || !scene) return;

        Entity entity{ entt_entity, registry };
        auto& tc = entity.GetComponent<TransformComponent>();
        PxTransform pose(ToPx(tc.Position), ToPx(glm::quat(tc.Rotation)));

        PxMaterial* mat = mMaterial ? mMaterial : (mMaterial = phys.GetPhysics()->createMaterial(0.5f, 0.5f, 0.1f));
        if (mActor)
        {
            PhysicEngine::Instance().RemoveActor(*mActor);
            if (mShape) { mShape->release(); mShape = nullptr; }
            mActor->release();
            mActor = nullptr;
            mDynamic = nullptr;
        }

        mCurrentType = ParseBodyType(bodyTypeString);
        if (mCurrentType == BodyType::Static)
        {
            PxRigidStatic* a = sdk->createRigidStatic(pose);
            mShape = sdk->createShape(PxBoxGeometry(ToPx(halfExtents)), *mat, true);
            a->attachShape(*mShape);
            mActor = a;
        }
        else
        {
            PxRigidDynamic* a = sdk->createRigidDynamic(pose);
            mShape = sdk->createShape(PxBoxGeometry(ToPx(halfExtents)), *mat, true);
            a->attachShape(*mShape);
            PxRigidBodyExt::updateMassAndInertia(*a, density);
            if (mCurrentType == BodyType::Kinematic) a->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            mActor = a;
            mDynamic = a;
        }

        //mActor->setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);

        PhysicEngine::Instance().AddActor(*mActor);
        UpdateEnableGravity();
        UpdateDamping();
        UpdateLinearAxisFactor();
        UpdateAngularAxisFactor();
    }

    void RigidBodyComponent::Init()
    {
        RebuildActor();
    }

    void RigidBodyComponent::Update(float)
    {
        if (!mActor) return;
        Entity entity{ entt_entity, registry };
        auto& tc = entity.GetComponent<TransformComponent>();

        const PxTransform p = mActor->getGlobalPose();
        tc.Position = ToGlm(p.p);
        tc.Rotation = glm::eulerAngles(ToGlm(p.q));
    }

    void RigidBodyComponent::UpdateEnableGravity()
    {
        if (!mActor) return;
        mActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !enableGravity);
    }

    void RigidBodyComponent::UpdateDamping()
    {
        if (!mDynamic) return;
        mDynamic->setLinearDamping(linearDamping);
        mDynamic->setAngularDamping(angularDamping);
    }

    void RigidBodyComponent::UpdateBodyType()
    {
        if (!mActor) { RebuildActor(); return; }
        const BodyType desired = ParseBodyType(bodyTypeString);
        if (desired == mCurrentType) return;
        RebuildActor();
    }

    void RigidBodyComponent::UpdateLinearAxisFactor()
    {
        if (!mDynamic) return;
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, !m_LinearAxisFactorX);
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, !m_LinearAxisFactorY);
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, !m_LinearAxisFactorZ);
    }

    void RigidBodyComponent::UpdateAngularAxisFactor()
    {
        if (!mDynamic) return;
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, !m_AngularAxisFactorX);
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, !m_AngularAxisFactorY);
        mDynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, !m_AngularAxisFactorZ);
    }
}
