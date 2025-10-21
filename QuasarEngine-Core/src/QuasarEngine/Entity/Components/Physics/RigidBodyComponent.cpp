#include "qepch.h"
#include "RigidBodyComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

namespace QuasarEngine
{
    RigidBodyComponent::RigidBodyComponent() {}

    RigidBodyComponent::~RigidBodyComponent()
    {
        Destroy();
    }

    RigidBodyComponent::RigidBodyComponent(const RigidBodyComponent& other)
    {
        bodyTypeString = other.bodyTypeString;
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        mActor = nullptr;
        mDynamic = nullptr;
        mCurrentType = ParseBodyType(bodyTypeString);
    }

    RigidBodyComponent& RigidBodyComponent::operator=(const RigidBodyComponent& other)
    {
        if (this == &other) return *this;

        Destroy();

        bodyTypeString = other.bodyTypeString;
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        mActor = nullptr;
        mDynamic = nullptr;
        mCurrentType = ParseBodyType(bodyTypeString);

        return *this;
    }

    RigidBodyComponent::RigidBodyComponent(RigidBodyComponent&& other) noexcept
    {
        bodyTypeString = std::move(other.bodyTypeString);
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        mActor = other.mActor;   other.mActor = nullptr;
        mDynamic = other.mDynamic; other.mDynamic = nullptr;
        mCurrentType = other.mCurrentType;
    }

    RigidBodyComponent& RigidBodyComponent::operator=(RigidBodyComponent&& other) noexcept
    {
        if (this == &other) return *this;

        Destroy();

        bodyTypeString = std::move(other.bodyTypeString);
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        mActor = other.mActor;   other.mActor = nullptr;
        mDynamic = other.mDynamic; other.mDynamic = nullptr;
        mCurrentType = other.mCurrentType;

        return *this;
    }

    void RigidBodyComponent::Destroy()
    {
        if (!mActor) return;

        physx::PxScene* scene = PhysicEngine::Instance().GetScene();
        if (scene)
        {
            scene->lockWrite();
            scene->removeActor(*mActor);
            scene->unlockWrite();
        }

        mActor->userData = nullptr;
        mActor->release();
        mActor = nullptr;
        mDynamic = nullptr;
    }

    RigidBodyComponent::BodyType RigidBodyComponent::ParseBodyType(const std::string& s) const
    {
        if (s == "STATIC")    return BodyType::Static;
        if (s == "KINEMATIC") return BodyType::Kinematic;
        return BodyType::Dynamic;
    }

    void RigidBodyComponent::RebuildActor()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxPhysics* sdk = phys.GetPhysics();
        physx::PxScene* scene = phys.GetScene();
        if (!sdk || !scene) return;

        if (mActor)
        {
            scene->lockWrite();
            scene->removeActor(*mActor);
            scene->unlockWrite();

            mActor->userData = nullptr;
            mActor->release();
            mActor = nullptr;
            mDynamic = nullptr;
        }

        Entity entity{ entt_entity, registry };
        auto& tc = entity.GetComponent<TransformComponent>();
        const physx::PxTransform pose(ToPx(tc.Position), ToPx(glm::quat(tc.Rotation)));

        mCurrentType = ParseBodyType(bodyTypeString);
        if (mCurrentType == BodyType::Static)
        {
            physx::PxRigidStatic* a = sdk->createRigidStatic(pose);
            mActor = a;
        }
        else
        {
            physx::PxRigidDynamic* a = sdk->createRigidDynamic(pose);
            a->setSolverIterationCounts(8, 2);
            physx::PxRigidBodyExt::updateMassAndInertia(*a, density);
            if (mCurrentType == BodyType::Kinematic)
                a->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            mActor = a;
            mDynamic = a;
        }

        mActor->setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);
        mActor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entt_entity));

        scene->lockWrite();
        scene->addActor(*mActor);
        scene->unlockWrite();

        UpdateEnableGravity();
        UpdateDamping();
        UpdateLinearAxisFactor();
        UpdateAngularAxisFactor();
    }

    void RigidBodyComponent::Init()
    {
        RebuildActor();
    }

    void RigidBodyComponent::Update(double dt)
    {
        if (!mActor) return;

        physx::PxScene* scene = PhysicEngine::Instance().GetScene();
        if (!scene) return;

        scene->lockRead();
        const physx::PxTransform p = mActor->getGlobalPose();
        scene->unlockRead();

        Entity entity{ entt_entity, registry };
        if (entity.IsValid())
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            tc.Position = ToGlm(p.p);
            tc.Rotation = glm::eulerAngles(ToGlm(p.q));
        }
    }

    void RigidBodyComponent::UpdateEnableGravity()
    {
        if (!mActor) return;
        mActor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !enableGravity);
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
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, !m_LinearAxisFactorX);
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, !m_LinearAxisFactorY);
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, !m_LinearAxisFactorZ);
    }

    void RigidBodyComponent::UpdateAngularAxisFactor()
    {
        if (!mDynamic) return;
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, !m_AngularAxisFactorX);
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, !m_AngularAxisFactorY);
        mDynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, !m_AngularAxisFactorZ);
    }
}