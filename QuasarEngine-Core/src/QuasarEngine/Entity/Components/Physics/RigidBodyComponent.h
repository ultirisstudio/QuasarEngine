#pragma once

#include <QuasarEngine/Entity/Component.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PxPhysicsAPI.h>

#include <string>
#include <algorithm>
#include <utility>

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

        RigidBodyComponent(const RigidBodyComponent& other);
        RigidBodyComponent& operator=(const RigidBodyComponent& other);

        RigidBodyComponent(RigidBodyComponent&& other) noexcept;
        RigidBodyComponent& operator=(RigidBodyComponent&& other) noexcept;

        void Init();
        void Update(double dt);
        void Destroy();

        void UpdateEnableGravity();
        void UpdateBodyType();
        void UpdateLinearAxisFactor();
        void UpdateAngularAxisFactor();
        void UpdateDamping();

        bool MoveKinematic(const glm::vec3& targetPos, const glm::quat& targetRot, bool doSweep);

        void SetKinematicTarget(const glm::vec3& targetPos, const glm::quat& targetRot);

        physx::PxRigidActor* GetActor() const noexcept { return m_Actor; }
        physx::PxRigidDynamic* GetDynamic() const noexcept { return m_Dynamic; }

        bool IsStatic() const noexcept { return m_Actor && (m_Dynamic == nullptr); }
        bool IsDynamic() const noexcept { return m_Dynamic && !m_Dynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC); }
        bool IsKinematic() const noexcept { return m_Dynamic && m_Dynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC); }

    private:
        enum class BodyType { Dynamic, Static, Kinematic };

        BodyType ParseBodyType(const std::string& s) const;
        void RebuildActor();

        physx::PxRigidActor* m_Actor = nullptr;
        physx::PxRigidDynamic* m_Dynamic = nullptr;

        BodyType mCurrentType = BodyType::Dynamic;
    };
}