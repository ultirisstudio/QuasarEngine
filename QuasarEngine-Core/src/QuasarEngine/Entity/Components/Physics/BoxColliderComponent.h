#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class BoxColliderComponent : public PrimitiveColliderComponent
    {
    public:
        BoxColliderComponent();
        ~BoxColliderComponent() override;

        BoxColliderComponent(const BoxColliderComponent&) = delete;
        BoxColliderComponent& operator=(const BoxColliderComponent&) = delete;
        BoxColliderComponent(BoxColliderComponent&&) = default;
        BoxColliderComponent& operator=(BoxColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        bool  m_UseEntityScale = true;
        glm::vec3 m_Size = { 1.f, 1.f, 1.f };

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;
    };
}