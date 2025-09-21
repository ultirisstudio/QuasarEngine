#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

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

        void SetMesh(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices);
        const std::vector<glm::vec3>& GetVertices() const { return mVertices; }
        const std::vector<uint32_t>& GetIndices()  const { return mIndices; }

        bool m_UseEntityScale = true;

        physx::PxShape* GetShape()    const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial() const noexcept { return mMaterial; }
        physx::PxTriangleMesh* GetMesh()     const noexcept { return mTriMesh; }

    private:
        void AttachOrRebuild();

        std::vector<glm::vec3> mVertices;
        std::vector<uint32_t>  mIndices;
        bool mDirty = true;

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
        physx::PxTriangleMesh* mTriMesh = nullptr;
    };
}