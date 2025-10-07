#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>
#include <glm/glm.hpp>
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

        void OnActorAboutToBeReleased(physx::PxRigidActor& actor);

        bool       m_UseEntityScale = true;
        glm::vec3  m_Size = { 1.f, 1.f, 1.f };

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;
        physx::PxRigidActor* m_AttachedActor = nullptr;

        bool m_Destroyed = false;
        bool m_OwnsMaterial = false;
    };
}