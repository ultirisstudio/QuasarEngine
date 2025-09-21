#include "qepch.h"

#include "ConvexMeshColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    static inline PxVec3 ToPx(const glm::vec3& v) { return PxVec3(v.x, v.y, v.z); }

    ConvexMeshColliderComponent::ConvexMeshColliderComponent() {}
    ConvexMeshColliderComponent::~ConvexMeshColliderComponent()
    {
        if (mShape) { mShape->release(); mShape = nullptr; }
        if (mConvex) { mConvex->release(); mConvex = nullptr; }
        if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void ConvexMeshColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void ConvexMeshColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor || mPoints.empty()) return;

        if (!mDirty && mShape) return;

        if (mShape) { actor->detachShape(*mShape); mShape->release(); mShape = nullptr; }
        if (mConvex) { mConvex->release(); mConvex = nullptr; }

        std::vector<PxVec3> pts; pts.reserve(mPoints.size());
        glm::vec3 scale(1.f);
        if (m_UseEntityScale) scale = entity.GetComponent<TransformComponent>().Scale;
        for (auto& p : mPoints) pts.emplace_back(p.x * scale.x, p.y * scale.y, p.z * scale.z);

        PxConvexMeshDesc desc;
        desc.points.count = static_cast<uint32_t>(pts.size());
        desc.points.stride = sizeof(PxVec3);
        desc.points.data = pts.data();
        desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

        mConvex = PxCreateConvexMesh(phys.GetPhysics()->getTolerancesScale(), desc);
        if (!mConvex) return;

        PxConvexMeshGeometry geom(mConvex);
        mShape = sdk->createShape(geom, *mMaterial, true);
        if (!mShape) return;

        actor->attachShape(*mShape);
        mDirty = false;
        RecomputeMass();
    }

    void ConvexMeshColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
        RecomputeMass();
    }

    void ConvexMeshColliderComponent::UpdateColliderSize()
    {
        mDirty = true;
        AttachOrRebuild();
    }

    void ConvexMeshColliderComponent::RecomputeMass()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !mShape) return;

        const float densityVal = std::max(0.000001f, mass);
        PxRigidBodyExt::updateMassAndInertia(*dyn, densityVal);
        dyn->setMass(mass);
    }
}