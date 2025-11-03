#pragma once

#include <PxPhysicsAPI.h>
#include <PxQueryFiltering.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace QuasarEngine
{
    inline physx::PxVec3 ToPx(const glm::vec3& v) { return { v.x, v.y, v.z }; }
    inline glm::vec3     ToGlm(const physx::PxVec3& v) { return { v.x, v.y, v.z }; }

    inline physx::PxQuat ToPx(const glm::quat& q) { return { q.x, q.y, q.z, q.w }; }
    inline glm::quat     ToGlm(const physx::PxQuat& q) { return { q.w, q.x, q.y, q.z }; }

    inline physx::PxTransform ToPx(const glm::vec3& p, const glm::quat& r) {
        return physx::PxTransform(ToPx(p), ToPx(r));
    }

    struct QueryOptions {
        uint32_t layer = 0xFFFFFFFF;
        uint32_t mask = 0xFFFFFFFF;

        physx::PxQueryFlags queryFlags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;

        physx::PxHitFlags hitFlags = physx::PxHitFlag::eDEFAULT;

        bool includeTriggers = false;

        bool bothSides = false;

        bool preciseSweep = false;
    };

    struct QueryFilterCB final : physx::PxQueryFilterCallback {
        bool includeTriggers = false;
        const physx::PxRigidActor* ignoreActor = nullptr;

        physx::PxQueryHitType::Enum preFilter(
            const physx::PxFilterData& filterData,
            const physx::PxShape* shape,
            const physx::PxRigidActor* actor,
            physx::PxHitFlags& queryFlags) override
        {
            const physx::PxFilterData& fd = shape->getQueryFilterData();

            const bool layerPass =
                ((fd.word0 & filterData.word1) != 0) &&
                ((filterData.word0 & fd.word1) != 0);
            if (!layerPass)
                return physx::PxQueryHitType::Enum::eNONE;

            if (ignoreActor && actor == ignoreActor)
                return physx::PxQueryHitType::Enum::eNONE;

            if (!includeTriggers && shape->getFlags().isSet(physx::PxShapeFlag::eTRIGGER_SHAPE))
                return physx::PxQueryHitType::Enum::eNONE;

            return physx::PxQueryHitType::Enum::eBLOCK;
        }

        physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit, const physx::PxShape* shape, const physx::PxRigidActor* actor) override
        {
            return physx::PxQueryHitType::Enum::eBLOCK;
        }
    };

    inline physx::PxQueryFilterData MakeFilterData(const QueryOptions& opts) {
        physx::PxQueryFilterData qfd;
        qfd.flags = opts.queryFlags | physx::PxQueryFlag::ePREFILTER;
        qfd.data = physx::PxFilterData(opts.layer, opts.mask, 0, 0);
        return qfd;
    }
}