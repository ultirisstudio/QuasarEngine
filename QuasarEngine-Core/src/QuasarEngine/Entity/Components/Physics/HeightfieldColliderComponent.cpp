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
        if (m_Shape) { m_Shape->release();       m_Shape = nullptr; }
        if (m_HeightField) { m_HeightField->release(); m_HeightField = nullptr; }
        if (m_Material) { m_Material->release();    m_Material = nullptr; }
    }

    void HeightfieldColliderComponent::SetHeightData(uint32_t rows, uint32_t cols,
        const std::vector<float>& heightsMeters,
        float cellSizeX, float cellSizeZ)
    {
        m_Rows = rows;
        m_Cols = cols;
        m_Heights = heightsMeters;
        m_CellSizeX = std::max(1e-6f, cellSizeX);
        m_CellSizeZ = std::max(1e-6f, cellSizeZ);
        m_Dirty = true;
    }

    void HeightfieldColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!m_Material) m_Material = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void HeightfieldColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk || m_Rows < 2 || m_Cols < 2 || m_Heights.size() != size_t(m_Rows) * size_t(m_Cols)) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        if (rb.GetDynamic()) return;
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        if (!m_Dirty && m_Shape) return;

        if (m_Shape) { actor->detachShape(*m_Shape); m_Shape->release(); m_Shape = nullptr; }
        if (m_HeightField) { m_HeightField->release();     m_HeightField = nullptr; }

        BuildHeightField();
        if (!m_HeightField) return;

        glm::vec3 entScale(1.f);
        if (m_UseEntityScale) entScale = entity.GetComponent<TransformComponent>().Scale;

        const float rowScale = m_CellSizeZ * std::max(1e-6f, entScale.z);
        const float columnScale = m_CellSizeX * std::max(1e-6f, entScale.x);
        const float heightScale = m_HeightScale * std::max(1e-6f, entScale.y);

        PxHeightFieldGeometry geom(m_HeightField, PxMeshGeometryFlags(), heightScale, rowScale, columnScale);
        m_Shape = sdk->createShape(geom, *m_Material, true);
        if (!m_Shape) return;

        m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
        m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        m_Shape->setLocalPose(physx::PxTransform(
            physx::PxVec3(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z),
            physx::PxQuat(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, m_LocalRotation.w)));
        physx::PxFilterData qfd; qfd.word0 = 0xFFFFFFFF; qfd.word1 = 0xFFFFFFFF; m_Shape->setQueryFilterData(qfd);

        actor->attachShape(*m_Shape);
        m_Dirty = false;
    }

    void HeightfieldColliderComponent::SetTrigger(bool isTrigger)
    {
        m_IsTrigger = isTrigger;
        if (m_Shape) {
            m_Shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_IsTrigger);
            m_Shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_IsTrigger);
        }
    }

    void HeightfieldColliderComponent::SetLocalPose(const glm::vec3& p, const glm::quat& r)
    {
        m_LocalPosition = p;
        m_LocalRotation = r;
        if (m_Shape) {
            m_Shape->setLocalPose(physx::PxTransform(
                physx::PxVec3(p.x, p.y, p.z),
                physx::PxQuat(r.x, r.y, r.z, r.w)));
        }
    }

    void HeightfieldColliderComponent::SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
        physx::PxCombineMode::Enum restitution)
    {
        m_FrictionCombine = friction;
        m_RestitutionCombine = restitution;
        UpdateColliderMaterial();
    }

    void HeightfieldColliderComponent::OnActorAboutToBeReleased(physx::PxRigidActor& actor)
    {
        if (!m_Shape) return;

        if (auto* scene = PhysicEngine::Instance().GetScene()) {
            PxWriteLockGuard lock(scene);
            actor.detachShape(*m_Shape);
        }
        else {
            actor.detachShape(*m_Shape);
        }

        m_Shape->release();
        m_Shape = nullptr;
    }

    void HeightfieldColliderComponent::UpdateColliderMaterial()
    {
        if (!m_Material) return;
        m_Material->setStaticFriction(friction);
        m_Material->setDynamicFriction(friction);
        m_Material->setRestitution(bounciness);
        m_Material->setFrictionCombineMode(m_FrictionCombine);
        m_Material->setRestitutionCombineMode(m_RestitutionCombine);
    }

    void HeightfieldColliderComponent::BuildHeightField()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        float maxAbs = 0.f;
        for (float h : m_Heights) maxAbs = std::max(maxAbs, std::abs(h));
        m_HeightScale = std::max(1e-4f, maxAbs / 30000.f);

        std::vector<PxHeightFieldSample> samples(size_t(m_Rows) * size_t(m_Cols));
        for (uint32_t r = 0; r < m_Rows; ++r)
        {
            for (uint32_t c = 0; c < m_Cols; ++c)
            {
                const float h = m_Heights[size_t(r) * m_Cols + c];
                int32_t q = int32_t(std::lround(h / m_HeightScale));
                q = std::max<int32_t>(-32767, std::min<int32_t>(32767, q));
                PxHeightFieldSample& s = samples[size_t(r) * m_Cols + c];
                s.height = static_cast<PxI16>(q);
                s.materialIndex0 = 0;
                s.materialIndex1 = 0;
                s.clearTessFlag();
            }
        }

        PxHeightFieldDesc desc{};
        desc.nbRows = m_Rows;
        desc.nbColumns = m_Cols;
        desc.samples.data = samples.data();
        desc.samples.stride = sizeof(PxHeightFieldSample);

        m_HeightField = PxCreateHeightField(desc);
    }

    void HeightfieldColliderComponent::UpdateColliderSize()
    {
        m_Dirty = true;
        AttachOrRebuild();
    }

    void HeightfieldColliderComponent::SetQueryFilter(uint32_t layer, uint32_t mask)
    {
        if (!m_Shape) return;
        SetFilterDataOnShape(*m_Shape, layer, mask);
    }
}