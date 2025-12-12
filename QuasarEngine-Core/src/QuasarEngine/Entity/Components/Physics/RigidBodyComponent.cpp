#include "qepch.h"
#include "RigidBodyComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Physic/PhysXQueryUtils.h>
#include "BoxColliderComponent.h"
#include "SphereColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "ConvexMeshColliderComponent.h"
#include "TriangleMeshColliderComponent.h"
#include "HeightfieldColliderComponent.h"
#include "PlaneColliderComponent.h"

namespace QuasarEngine
{
    namespace
    {
        static void NotifyCollidersActorAboutToBeReleased(Entity& entity, physx::PxRigidActor& actor)
        {
            if (entity.HasComponent<BoxColliderComponent>())
                entity.GetComponent<BoxColliderComponent>().OnActorAboutToBeReleased(actor);

            if (entity.HasComponent<CapsuleColliderComponent>())
                entity.GetComponent<CapsuleColliderComponent>().OnActorAboutToBeReleased(actor);
        }

        static void ReattachPrimitiveColliders(Entity& entity)
        {
            if (entity.HasComponent<BoxColliderComponent>())
                entity.GetComponent<BoxColliderComponent>().UpdateColliderSize();

            if (entity.HasComponent<CapsuleColliderComponent>())
                entity.GetComponent<CapsuleColliderComponent>().UpdateColliderSize();
        }
    }

    RigidBodyComponent::RigidBodyComponent() {}

    RigidBodyComponent::~RigidBodyComponent()
    {
        Destroy();
    }

    RigidBodyComponent::RigidBodyComponent(const RigidBodyComponent& other)
    {
        bodyTypeString = other.bodyTypeString;
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        m_Actor = nullptr;
        m_Dynamic = nullptr;
        mCurrentType = ParseBodyType(bodyTypeString);
    }

    RigidBodyComponent& RigidBodyComponent::operator=(const RigidBodyComponent& other)
    {
        if (this == &other) return *this;

        Destroy();

        bodyTypeString = other.bodyTypeString;
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        m_Actor = nullptr;
        m_Dynamic = nullptr;
        mCurrentType = ParseBodyType(bodyTypeString);

        return *this;
    }

    RigidBodyComponent::RigidBodyComponent(RigidBodyComponent&& other) noexcept
    {
        bodyTypeString = std::move(other.bodyTypeString);
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        m_Actor = other.m_Actor;   other.m_Actor = nullptr;
        m_Dynamic = other.m_Dynamic; other.m_Dynamic = nullptr;
        mCurrentType = other.mCurrentType;
    }

    RigidBodyComponent& RigidBodyComponent::operator=(RigidBodyComponent&& other) noexcept
    {
        if (this == &other) return *this;

        Destroy();

        bodyTypeString = std::move(other.bodyTypeString);
        enableGravity = other.enableGravity;

        m_LinearAxisFactorX = other.m_LinearAxisFactorX;
        m_LinearAxisFactorY = other.m_LinearAxisFactorY;
        m_LinearAxisFactorZ = other.m_LinearAxisFactorZ;

        m_AngularAxisFactorX = other.m_AngularAxisFactorX;
        m_AngularAxisFactorY = other.m_AngularAxisFactorY;
        m_AngularAxisFactorZ = other.m_AngularAxisFactorZ;

        linearDamping = other.linearDamping;
        angularDamping = other.angularDamping;
        density = other.density;

        m_Actor = other.m_Actor;   other.m_Actor = nullptr;
        m_Dynamic = other.m_Dynamic; other.m_Dynamic = nullptr;
        mCurrentType = other.mCurrentType;

        return *this;
    }

    void RigidBodyComponent::Destroy()
    {
        if (!m_Actor) return;

        if (physx::PxScene* sc = m_Actor->getScene()) {
            PxWriteLockGuard _lock(sc);
            sc->removeActor(*m_Actor);

            Entity entity{ entt_entity, registry };
            if (entity.IsValid())
                NotifyCollidersActorAboutToBeReleased(entity, *m_Actor);
        }

        PhysicEngine::Instance().UnregisterActor(m_Actor);

        m_Actor->userData = nullptr;
        m_Actor->release();
        m_Actor = nullptr;
        m_Dynamic = nullptr;
    }

    RigidBodyComponent::BodyType RigidBodyComponent::ParseBodyType(const std::string& s) const
    {
        if (s == "STATIC")    return BodyType::Static;
        if (s == "KINEMATIC") return BodyType::Kinematic;
        return BodyType::Dynamic;
    }

    void RigidBodyComponent::RebuildActor()
    {
        auto& phys = PhysicEngine::Instance();
        physx::PxPhysics* sdk = phys.GetPhysics();
        physx::PxScene* scene = phys.GetScene();
        if (!sdk || !scene) return;

        Entity entity{ entt_entity, registry };

        if (m_Actor) {
            if (physx::PxScene* sc = m_Actor->getScene()) {
                sc->lockWrite();
                sc->removeActor(*m_Actor);
                if (entity.IsValid())
                    NotifyCollidersActorAboutToBeReleased(entity, *m_Actor);
                sc->unlockWrite();
            }
            phys.UnregisterActor(m_Actor);
            m_Actor->userData = nullptr;
            m_Actor->release();
            m_Actor = nullptr;
            m_Dynamic = nullptr;
        }

        auto& tc = entity.GetComponent<TransformComponent>();
        const physx::PxTransform pose(ToPx(tc.Position), ToPx(glm::quat(tc.Rotation)));

        mCurrentType = ParseBodyType(bodyTypeString);
        if (mCurrentType == BodyType::Static)
        {
            physx::PxRigidStatic* a = sdk->createRigidStatic(pose);
            m_Actor = a;
            m_Dynamic = nullptr;
        }
        else
        {
            physx::PxRigidDynamic* a = sdk->createRigidDynamic(pose);
            a->setSolverIterationCounts(8, 2);
            if (mCurrentType == BodyType::Kinematic)
                a->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            m_Actor = a;
            m_Dynamic = a;
        }

        m_Actor->setActorFlag(physx::PxActorFlag::eVISUALIZATION, true);

        phys.RegisterActor(m_Actor, entt_entity);

        scene->lockWrite();
        scene->addActor(*m_Actor);
        scene->unlockWrite();

        UpdateEnableGravity();
        UpdateDamping();
        UpdateLinearAxisFactor();
        UpdateAngularAxisFactor();

        if (entity.IsValid())
            ReattachPrimitiveColliders(entity);
    }

    void RigidBodyComponent::Init()
    {
        RebuildActor();
    }

    void RigidBodyComponent::Update(double dt)
    {
        if (!m_Actor) return;

        //physx::PxScene* scene = PhysicEngine::Instance().GetScene();
        //if (!scene) return;

        //scene->lockRead();
        const physx::PxTransform p = m_Actor->getGlobalPose();
        //scene->unlockRead();

        Entity entity{ entt_entity, registry };
        if (entity.IsValid())
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            tc.Position = ToGlm(p.p);
            tc.Rotation = glm::eulerAngles(ToGlm(p.q));
        }
    }

    void RigidBodyComponent::UpdateEnableGravity()
    {
        if (!m_Actor) return;
        m_Actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !enableGravity);
    }

    void RigidBodyComponent::UpdateDamping()
    {
        if (!m_Dynamic) return;
        m_Dynamic->setLinearDamping(linearDamping);
        m_Dynamic->setAngularDamping(angularDamping);
    }

    void RigidBodyComponent::SetKinematicTarget(const glm::vec3& p, const glm::quat& r)
    {
        if (!m_Dynamic) return;
        if (!m_Dynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC)) return;
        m_Dynamic->setKinematicTarget(physx::PxTransform(ToPx(p), ToPx(r)));
    }

    bool RigidBodyComponent::MoveKinematic(const glm::vec3& targetPos,
        const glm::quat& targetRot,
        bool doSweep)
    {
        using namespace physx;

        if (!m_Dynamic) return false;
        if (!m_Dynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return false;

        const PxTransform to(ToPx(targetPos), ToPx(targetRot));

        if (!doSweep) {
            m_Dynamic->setKinematicTarget(to);
            return true;
        }

        PxU32 nbShapes = m_Dynamic->getNbShapes();
        if (nbShapes == 0) {
            m_Dynamic->setKinematicTarget(to);
            return true;
        }

        std::vector<PxShape*> shapes(nbShapes);
        m_Dynamic->getShapes(shapes.data(), nbShapes);
        PxShape* shape = shapes[0];

        const PxTransform fromActor = m_Dynamic->getGlobalPose();
        const PxTransform shapeLocal = shape->getLocalPose();
        const PxTransform from = fromActor * shapeLocal;
        const PxTransform toShape = to * shapeLocal;
        const PxVec3 delta = (toShape.p - from.p);

        const float dist = delta.magnitude();

        if (dist <= 1e-4f) {
            m_Dynamic->setKinematicTarget(to);
            return true;
        }

        const PxVec3 unitDir = delta.getNormalized();

        PxGeometryHolder gh = shape->getGeometry();
        PxSweepBuffer buf;

        QueryOptions opts;
        opts.hitFlags = PxHitFlag::eDEFAULT | PxHitFlag::eMTD;
        opts.includeTriggers = false;
        opts.preciseSweep = true;

        PxHitFlags hitFlags = opts.hitFlags;
        if (opts.preciseSweep) hitFlags |= PxHitFlag::ePRECISE_SWEEP;

        QueryFilterCB cb;
        cb.includeTriggers = false;
        cb.ignoreActor = m_Dynamic;

        PxQueryFilterData qfd = MakeFilterData(opts);

        PxScene* scene = PhysicEngine::Instance().GetScene();
        if (!scene) {
            m_Dynamic->setKinematicTarget(to);
            return true;
        }

        const bool anyHit = scene->sweep(gh.any(), from, unitDir, dist, buf, hitFlags, qfd, &cb, nullptr);

        if (!anyHit || !buf.hasBlock) {
            m_Dynamic->setKinematicTarget(to);
            return true;
        }

        const PxSweepHit& h = buf.block;

        const float safety = 0.01f;
        const float travel = PxMax<PxReal>(0.0f, h.distance - safety);

        PxTransform safeShape = from;
        safeShape.p += unitDir * travel;
        const PxTransform safeActor = safeShape * shapeLocal.getInverse();
        m_Dynamic->setKinematicTarget(safeActor);

        return false;
    }

    void RigidBodyComponent::UpdateBodyType()
    {
        if (!m_Actor) { RebuildActor(); return; }
        const BodyType desired = ParseBodyType(bodyTypeString);
        if (desired == mCurrentType) return;
        RebuildActor();
    }

    void RigidBodyComponent::UpdateLinearAxisFactor()
    {
        if (!m_Dynamic) return;
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, !m_LinearAxisFactorX);
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, !m_LinearAxisFactorY);
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, !m_LinearAxisFactorZ);
    }

    void RigidBodyComponent::UpdateAngularAxisFactor()
    {
        if (!m_Dynamic) return;
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, !m_AngularAxisFactorX);
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, !m_AngularAxisFactorY);
        m_Dynamic->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, !m_AngularAxisFactorZ);
    }
}