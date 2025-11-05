#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

        void SetQueryFilter(uint32_t layer, uint32_t mask);

        void SetTrigger(bool isTrigger);
        bool IsTrigger() const noexcept { return m_IsTrigger; }

        void SetLocalPose(const glm::vec3& localPosition, const glm::quat& localRotation);

        glm::vec3 GetLocalPosition() const noexcept { return m_LocalPosition; }
        glm::quat GetLocalRotation() const noexcept { return m_LocalRotation; }

        void SetMaterialCombineModes(physx::PxCombineMode::Enum friction, physx::PxCombineMode::Enum restitution);

        void OnActorAboutToBeReleased(physx::PxRigidActor& actor);

        void SetHeightData(uint32_t rows, uint32_t cols,
            const std::vector<float>& heightsMeters,
            float cellSizeX = 1.f, float cellSizeZ = 1.f);

        bool  m_UseEntityScale = true;

        physx::PxShape* GetShape()        const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial()     const noexcept { return m_Material; }
        physx::PxHeightField* GetHeightField()  const noexcept { return m_HeightField; }

        uint32_t GetRows()      const noexcept { return m_Rows; }
        uint32_t GetCols()      const noexcept { return m_Cols; }

        const std::vector<float>& GetHeights() const noexcept { return m_Heights; }

        float GetCellSizeX()    const noexcept { return m_CellSizeX; }
        float GetCellSizeZ()    const noexcept { return m_CellSizeZ; }

    private:
        void AttachOrRebuild();
        void BuildHeightField();

        uint32_t m_Rows = 0, m_Cols = 0;
        std::vector<float> m_Heights;
        float m_CellSizeX = 1.f;
        float m_CellSizeZ = 1.f;

        physx::PxHeightField* m_HeightField = nullptr;
        float m_HeightScale = 1.f;

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0 };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;

        bool m_Dirty = true;
    };
}