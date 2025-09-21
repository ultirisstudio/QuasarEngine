#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

#include <vector>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class HeightfieldColliderComponent : public PrimitiveColliderComponent
    {
    public:
        HeightfieldColliderComponent();
        ~HeightfieldColliderComponent() override;

        HeightfieldColliderComponent(const HeightfieldColliderComponent&) = delete;
        HeightfieldColliderComponent& operator=(const HeightfieldColliderComponent&) = delete;
        HeightfieldColliderComponent(HeightfieldColliderComponent&&) = default;
        HeightfieldColliderComponent& operator=(HeightfieldColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        void SetHeightData(uint32_t rows, uint32_t cols,
            const std::vector<float>& heightsMeters,
            float cellSizeX = 1.f, float cellSizeZ = 1.f);

        bool  m_UseEntityScale = true;

        physx::PxShape* GetShape()        const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial()     const noexcept { return mMaterial; }
        physx::PxHeightField* GetHeightField()  const noexcept { return mHeightField; }

        uint32_t GetRows()      const noexcept { return mRows; }
        uint32_t GetCols()      const noexcept { return mCols; }
        const std::vector<float>& GetHeights() const noexcept { return mHeights; }
        float GetCellSizeX()    const noexcept { return mCellSizeX; }
        float GetCellSizeZ()    const noexcept { return mCellSizeZ; }

    private:
        void AttachOrRebuild();
        void BuildHeightField();

        uint32_t mRows = 0, mCols = 0;
        std::vector<float> mHeights;
        float mCellSizeX = 1.f;
        float mCellSizeZ = 1.f;

        physx::PxHeightField* mHeightField = nullptr;
        float mHeightScale = 1.f;

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;

        bool mDirty = true;
    };
}