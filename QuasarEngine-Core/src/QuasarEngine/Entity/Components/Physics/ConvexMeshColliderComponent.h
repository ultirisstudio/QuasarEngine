#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

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

        void SetPoints(const std::vector<glm::vec3>& pts) { mPoints = pts; mDirty = true; }
        const std::vector<glm::vec3>& GetPoints() const { return mPoints; }
        bool  m_UseEntityScale = true;

        physx::PxShape* GetShape()    const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial() const noexcept { return mMaterial; }
        physx::PxConvexMesh* GetMesh()     const noexcept { return mConvex; }

    private:
        void AttachOrRebuild();
        void RecomputeMass();

        std::vector<glm::vec3> mPoints;
        bool mDirty = true;

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
        physx::PxConvexMesh* mConvex = nullptr;
    };
}