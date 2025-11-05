#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class ConvexMeshColliderComponent : public PrimitiveColliderComponent
    {
    public:
        ConvexMeshColliderComponent();
        ~ConvexMeshColliderComponent() override;

        ConvexMeshColliderComponent(const ConvexMeshColliderComponent&) = delete;
        ConvexMeshColliderComponent& operator=(const ConvexMeshColliderComponent&) = delete;
        ConvexMeshColliderComponent(ConvexMeshColliderComponent&&) = default;
        ConvexMeshColliderComponent& operator=(ConvexMeshColliderComponent&&) = default;

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

        void SetPoints(const std::vector<glm::vec3>& pts) { m_Points = pts; m_Dirty = true; }
        const std::vector<glm::vec3>& GetPoints() const { return m_Points; }
        bool  m_UseEntityScale = true;

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }
        physx::PxConvexMesh* GetMesh()     const noexcept { return m_Convex; }

    private:
        void AttachOrRebuild();
        void RecomputeMass();

        std::vector<glm::vec3> m_Points;
        bool m_Dirty = true;

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;
        physx::PxConvexMesh* m_Convex = nullptr;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0 };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;
    };
}