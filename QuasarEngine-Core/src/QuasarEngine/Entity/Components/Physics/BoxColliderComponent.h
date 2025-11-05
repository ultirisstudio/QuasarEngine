#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class RigidBodyComponent;

    class BoxColliderComponent : public PrimitiveColliderComponent
    {
    public:
        BoxColliderComponent();
        ~BoxColliderComponent() override;

        BoxColliderComponent(BoxColliderComponent&& other) noexcept;
        BoxColliderComponent& operator=(BoxColliderComponent&& other) noexcept;

        BoxColliderComponent(const BoxColliderComponent& other);
        BoxColliderComponent& operator=(const BoxColliderComponent& other);

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;
        void Destroy();

        void SetQueryFilter(uint32_t layer, uint32_t mask);

        void SetTrigger(bool isTrigger);
        bool IsTrigger() const noexcept { return m_IsTrigger; }

        void SetLocalPose(const glm::vec3& localPosition, const glm::quat& localRotation);
        glm::vec3 GetLocalPosition() const noexcept { return m_LocalPosition; }
        glm::quat GetLocalRotation() const noexcept { return m_LocalRotation; }

        void SetMaterialCombineModes(physx::PxCombineMode::Enum friction, physx::PxCombineMode::Enum restitution);

        void OnActorAboutToBeReleased(physx::PxRigidActor& actor);

        bool m_UseEntityScale = true;
        glm::vec3  m_Size = { 1.f, 1.f, 1.f };

        physx::PxShape* GetShape() const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;
        physx::PxRigidActor* m_AttachedActor = nullptr;

        bool m_Destroyed = false;
        bool m_OwnsMaterial = false;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0.0f, 0.0f, 0.0f };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;
    };
}