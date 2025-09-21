#include "qepch.h"

#include "TriangleMeshColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    static inline PxVec3 ToPx(const glm::vec3& v) { return PxVec3(v.x, v.y, v.z); }

    TriangleMeshColliderComponent::TriangleMeshColliderComponent() {}
    TriangleMeshColliderComponent::~TriangleMeshColliderComponent()
    {
        if (mShape) { mShape->release(); mShape = nullptr; }
        if (mTriMesh) { mTriMesh->release(); mTriMesh = nullptr; }
        if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void TriangleMeshColliderComponent::SetMesh(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices)
    {
        mVertices = verts;
        mIndices = indices;
        mDirty = true;
    }

    void TriangleMeshColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void TriangleMeshColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk || mVertices.empty() || mIndices.empty()) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        if (rb.GetDynamic()) return;

        if (!mDirty && mShape) return;

        if (mShape) { actor->detachShape(*mShape); mShape->release(); mShape = nullptr; }
        if (mTriMesh) { mTriMesh->release(); mTriMesh = nullptr; }

        std::vector<PxVec3> pts; pts.reserve(mVertices.size());
        glm::vec3 scale(1.f);
        if (m_UseEntityScale) scale = entity.GetComponent<TransformComponent>().Scale;
        for (auto& v : mVertices) pts.emplace_back(v.x * scale.x, v.y * scale.y, v.z * scale.z);

        PxTriangleMeshDesc desc;
        desc.points.count = static_cast<uint32_t>(pts.size());
        desc.points.stride = sizeof(PxVec3);
        desc.points.data = pts.data();
        desc.triangles.count = static_cast<uint32_t>(mIndices.size() / 3);
        desc.triangles.stride = 3 * sizeof(uint32_t);
        desc.triangles.data = mIndices.data();
        desc.flags = PxMeshFlag::e16_BIT_INDICES;

        std::vector<uint16_t> idx16;

        if (mIndices.size() < (1u << 16))
        {
            idx16.resize(mIndices.size());
            for (size_t i = 0; i < mIndices.size(); ++i)
                idx16[i] = static_cast<uint16_t>(mIndices[i]);

            desc.flags = PxMeshFlag::e16_BIT_INDICES;
            desc.triangles.data = idx16.data();
            desc.triangles.stride = 3 * sizeof(uint16_t);
        }
        else
        {
            desc.triangles.data = mIndices.data();
            desc.triangles.stride = 3 * sizeof(uint32_t);
        }

        mTriMesh = PxCreateTriangleMesh(phys.GetPhysics()->getTolerancesScale(), desc);
        if (!mTriMesh) return;

        PxTriangleMeshGeometry geom(mTriMesh);
        mShape = sdk->createShape(geom, *mMaterial, true);
        if (!mShape) return;

        actor->attachShape(*mShape);
        mDirty = false;
    }

    void TriangleMeshColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
    }

    void TriangleMeshColliderComponent::UpdateColliderSize()
    {
        mDirty = true;
        AttachOrRebuild();
    }
}