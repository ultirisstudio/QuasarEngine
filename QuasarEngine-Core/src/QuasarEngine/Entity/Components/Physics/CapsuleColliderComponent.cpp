#include "qepch.h"

#include "CapsuleColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

using namespace physx;

namespace QuasarEngine
{
    static inline float Max3(float a, float b, float c) { return std::max(a, std::max(b, c)); }

    CapsuleColliderComponent::CapsuleColliderComponent() {}
    CapsuleColliderComponent::~CapsuleColliderComponent()
    {
        if (mShape) { mShape->release(); mShape = nullptr; }
        if (mMaterial) { mMaterial->release(); mMaterial = nullptr; }
    }

    void CapsuleColliderComponent::Init()
    {
        auto& phys = PhysicEngine::Instance();
        if (!mMaterial) mMaterial = phys.GetPhysics()->createMaterial(friction, friction, bounciness);
        AttachOrRebuild();
        UpdateColliderMaterial();
    }

    void CapsuleColliderComponent::AttachOrRebuild()
    {
        auto& phys = PhysicEngine::Instance();
        PxPhysics* sdk = phys.GetPhysics();
        if (!sdk) return;

        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidActor* actor = rb.GetActor();
        if (!actor) return;

        float scaleRadius = 1.f, scaleHeight = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            switch (m_Axis)
            {
            case Axis::X: scaleHeight = std::abs(s.x); scaleRadius = Max3(1e-4f, std::abs(s.y), std::abs(s.z)); break;
            case Axis::Y: scaleHeight = std::abs(s.y); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.z)); break;
            case Axis::Z: scaleHeight = std::abs(s.z); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.y)); break;
            }
        }

        const float radius = std::max(0.0001f, m_Radius * scaleRadius);
        const float halfHeight = std::max(0.0001f, 0.5f * m_Height * scaleHeight);

        PxCapsuleGeometry geom(radius, halfHeight);

        PxShape* newShape = sdk->createShape(geom, *mMaterial, true);
        if (!newShape) return;

        PxQuat rot = PxQuat(PxIdentity);
        if (m_Axis == Axis::Y)      rot = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
        else if (m_Axis == Axis::Z) rot = PxQuat(-PxHalfPi, PxVec3(0, 1, 0));
        newShape->setLocalPose(PxTransform(PxVec3(0), rot));

        if (mShape) { actor->detachShape(*mShape); mShape->release(); }
        mShape = newShape;
        actor->attachShape(*mShape);

        RecomputeMassFromSize();
    }

    void CapsuleColliderComponent::UpdateColliderMaterial()
    {
        if (!mMaterial) return;
        mMaterial->setStaticFriction(friction);
        mMaterial->setDynamicFriction(friction);
        mMaterial->setRestitution(bounciness);
        RecomputeMassFromSize();
    }

    void CapsuleColliderComponent::UpdateColliderSize()
    {
        AttachOrRebuild();
    }

    void CapsuleColliderComponent::RecomputeMassFromSize()
    {
        Entity entity{ entt_entity, registry };
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        PxRigidDynamic* dyn = rb.GetDynamic();
        if (!dyn || !mShape) return;

        float scaleRadius = 1.f, scaleHeight = 1.f;
        if (m_UseEntityScale)
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            switch (m_Axis)
            {
            case Axis::X: scaleHeight = std::abs(s.x); scaleRadius = Max3(1e-4f, std::abs(s.y), std::abs(s.z)); break;
            case Axis::Y: scaleHeight = std::abs(s.y); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.z)); break;
            case Axis::Z: scaleHeight = std::abs(s.z); scaleRadius = Max3(1e-4f, std::abs(s.x), std::abs(s.y)); break;
            }
        }

        const float r = std::max(0.0001f, m_Radius * scaleRadius);
        const float hh = std::max(0.0001f, 0.5f * m_Height * scaleHeight);
        const float cylinderHeight = 2.0f * hh;
        const float volume = (3.14159265358979323846f * r * r * cylinderHeight) + (4.0f / 3.0f) * 3.14159265358979323846f * r * r * r;
        const float densityVal = std::max(0.000001f, mass / volume);

        PxRigidBodyExt::updateMassAndInertia(*dyn, densityVal);
        dyn->setMass(mass);
    }
}