#pragma once

/*#include <glm/glm.hpp>
#include <QuasarEngine/Physic/Collision/Collider.h>


namespace QuasarEngine {

    class RaycastCallback {
    public:
        virtual ~RaycastCallback() = default;

        enum class RaycastResult {
            IgnoreCollider = -1,
            HitAndContinue = 1,
            ClipRay = 2,
            HitAndStop = 0
        };

        struct RaycastHit {
            Collider* collider = nullptr;
            float hitFraction = 0.0f;
            glm::vec3 point;
            glm::vec3 normal;
        };

        virtual RaycastResult NotifyRaycastHit(const RaycastHit& hit) = 0;
    };
}*/