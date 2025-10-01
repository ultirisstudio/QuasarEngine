#pragma once

#include <QuasarEngine/Entity/Component.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class RigidBodyComponent : public Component
    {
    public:
        std::string bodyTypeString = "DYNAMIC";
        bool enableGravity = true;

        bool m_LinearAxisFactorX = true;
        bool m_LinearAxisFactorY = true;
        bool m_LinearAxisFactorZ = true;

        bool m_AngularAxisFactorX = true;
        bool m_AngularAxisFactorY = true;
        bool m_AngularAxisFactorZ = true;

        float linearDamping = 0.01f;
        float angularDamping = 0.05f;

        float density = 10.f;

        RigidBodyComponent();
        ~RigidBodyComponent() override;

        RigidBodyComponent(const RigidBodyComponent&) = delete;
        RigidBodyComponent& operator=(const RigidBodyComponent&) = delete;
        RigidBodyComponent(RigidBodyComponent&&) = default;
        RigidBodyComponent& operator=(RigidBodyComponent&&) = default;

        void Destroy();
        void Init();
        void Update(float dt);

        void UpdateEnableGravity();
        void UpdateBodyType();
        void UpdateLinearAxisFactor();
        void UpdateAngularAxisFactor();
        void UpdateDamping();

        physx::PxRigidActor* GetActor() const noexcept { return mActor; }
        physx::PxRigidDynamic* GetDynamic() const noexcept { return mDynamic; }

        bool IsStatic()   const noexcept { return mActor && (mDynamic == nullptr); }
        bool IsDynamic()  const noexcept { return mDynamic && !mDynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC); }
        bool IsKinematic()const noexcept { return mDynamic && mDynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC); }

    private:
        enum class BodyType { Dynamic, Static, Kinematic };

        BodyType ParseBodyType(const std::string& s) const;
        void RebuildActor();

        physx::PxRigidActor* mActor = nullptr;
        physx::PxRigidDynamic* mDynamic = nullptr;
        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
        BodyType               mCurrentType = BodyType::Dynamic;
    };
}