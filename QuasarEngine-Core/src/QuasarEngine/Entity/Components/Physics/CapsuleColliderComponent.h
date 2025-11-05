#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class CapsuleColliderComponent : public PrimitiveColliderComponent
    {
    public:
        enum class Axis { X, Y, Z };

        CapsuleColliderComponent();
        ~CapsuleColliderComponent() override;

        CapsuleColliderComponent(const CapsuleColliderComponent&) = delete;
        CapsuleColliderComponent& operator=(const CapsuleColliderComponent&) = delete;
        CapsuleColliderComponent(CapsuleColliderComponent&&) = default;
        CapsuleColliderComponent& operator=(CapsuleColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        void SetQueryFilter(uint32_t layer, uint32_t mask);

        void SetTrigger(bool isTrigger);
        bool IsTrigger() const noexcept { return m_IsTrigger; }

        void SetLocalPose(const glm::vec3& localPosition, const glm::quat& localRotation);

        glm::vec3 GetLocalPosition() const noexcept { return m_LocalPosition; }
        glm::quat GetLocalRotation() const noexcept { return m_LocalRotation; }

        void SetMaterialCombineModes(physx::PxCombineMode::Enum friction, physx::PxCombineMode::Enum restitution);

        void OnActorAboutToBeReleased(physx::PxRigidActor& actor);

        bool  m_UseEntityScale = true;
        float m_Radius = 0.5f;
        float m_Height = 1.0f;
        Axis  m_Axis = Axis::Y;

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0 };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;
    };
}