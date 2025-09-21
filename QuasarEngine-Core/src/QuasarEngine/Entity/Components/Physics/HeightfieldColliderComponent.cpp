#include "qepch.h"

#include "HeightfieldColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

#include <algorithm>
#include <cmath>

using namespace physx;

namespace QuasarEngine
{
    HeightfieldColliderComponent::HeightfieldColliderComponent() {}
    HeightfieldColliderComponent::~HeightfieldColliderComponent()
    {
        if (mShape) { mShape->release();       mShape = nullptr; }
        if (mHeightField) { mHeightField->release(); mHeightField = nullptr; }
        if (mMaterial) { mMaterial->release();    mMaterial = nullptr; }
    }

    void HeightfieldColliderComponent::SetHeightData(uint32_t rows, uint32_t cols,
        const std::vector<float>& heightsMeters,
        float cellSizeX, float cellSizeZ)
    {
        mRows = rows;
        mCols = cols;
        mHeights = heightsMeters;
        mCellSizeX = std::max(1e-6f, cellSizeX);
        mCellSizeZ = std::max(1e-6f, cellSizeZ);
        mDirty = true;
    }

    void HeightfieldColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void HeightfieldColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk || mRows < 2 || mCols < 2 || mHeights.size() != size_t(mRows) * size_t(mCols)) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        if (rb.GetDynamic()) return;
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        if (!mDirty && mShape) return;

        if (mShape) { actor->detachShape(*mShape); mShape->release(); mShape = nullptr; }
        if (mHeightField) { mHeightField->release();     mHeightField = nullptr; }

        BuildHeightField();
        if (!mHeightField) return;

        glm::vec3 entScale(1.f);
        if (m_UseEntityScale) entScale = entity.GetComponent<TransformComponent>().Scale;

        const float rowScale = mCellSizeZ * std::max(1e-6f, entScale.z);
        const float columnScale = mCellSizeX * std::max(1e-6f, entScale.x);
        const float heightScale = mHeightScale * std::max(1e-6f, entScale.y);

        PxHeightFieldGeometry geom(mHeightField, PxMeshGeometryFlags(), heightScale, rowScale, columnScale);
        mShape = sdk->createShape(geom, *mMaterial, true);
        if (!mShape) return;

        actor->attachShape(*mShape);
        mDirty = false;
    }

    void HeightfieldColliderComponent::BuildHeightField()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        float maxAbs = 0.f;
        for (float h : mHeights) maxAbs = std::max(maxAbs, std::abs(h));
        mHeightScale = std::max(1e-4f, maxAbs / 30000.f);

        std::vector<PxHeightFieldSample> samples(size_t(mRows) * size_t(mCols));
        for (uint32_t r = 0; r < mRows; ++r)
        {
            for (uint32_t c = 0; c < mCols; ++c)
            {
                const float h = mHeights[size_t(r) * mCols + c];
                int32_t q = int32_t(std::lround(h / mHeightScale));
                q = std::max<int32_t>(-32767, std::min<int32_t>(32767, q));
                PxHeightFieldSample& s = samples[size_t(r) * mCols + c];
                s.height = static_cast<PxI16>(q);
                s.materialIndex0 = 0;
                s.materialIndex1 = 0;
                s.clearTessFlag();
            }
        }

        PxHeightFieldDesc desc{};
        desc.nbRows = mRows;
        desc.nbColumns = mCols;
        desc.samples.data = samples.data();
        desc.samples.stride = sizeof(PxHeightFieldSample);

        mHeightField = PxCreateHeightField(desc);
    }

    void HeightfieldColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
    }

    void HeightfieldColliderComponent::UpdateColliderSize()
    {
        mDirty = true;
        AttachOrRebuild();
    }
}