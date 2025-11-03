#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class TriangleMeshColliderComponent : public PrimitiveColliderComponent
    {
    public:
        TriangleMeshColliderComponent();
        ~TriangleMeshColliderComponent() override;

        TriangleMeshColliderComponent(const TriangleMeshColliderComponent&) = delete;
        TriangleMeshColliderComponent& operator=(const TriangleMeshColliderComponent&) = delete;
        TriangleMeshColliderComponent(TriangleMeshColliderComponent&&) = default;
        TriangleMeshColliderComponent& operator=(TriangleMeshColliderComponent&&) = default;

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

        void SetMesh(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices);
        const std::vector<glm::vec3>& GetVertices() const { return m_Vertices; }
        const std::vector<uint32_t>& GetIndices()  const { return m_Indices; }

        bool m_UseEntityScale = true;

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }
        physx::PxTriangleMesh* GetMesh()     const noexcept { return m_TriMesh; }

    private:
        void AttachOrRebuild();

        std::vector<glm::vec3> m_Vertices;
        std::vector<uint32_t>  m_Indices;
        bool mDirty = true;

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;
        physx::PxTriangleMesh* m_TriMesh = nullptr;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0 };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;
    };
}