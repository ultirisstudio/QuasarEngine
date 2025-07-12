#include "qepch.h"
/*#include "QuasarEngine.h"
#include "RaycastUtils.h"
#include <limits>
#include <glm/gtx/norm.hpp>


namespace QuasarEngine {

    bool RayIntersectsOBB(const RRay& ray,
        const glm::vec3& boxCenter,
        const glm::mat3& boxRotation,
        const glm::vec3& halfExtents,
        float& outDistance,
        glm::vec3& outHitPoint,
        glm::vec3& outNormal) {

        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        glm::vec3 p = boxCenter - ray.origin;

        for (int i = 0; i < 3; ++i) {
            glm::vec3 axis = boxRotation[i];
            float e = glm::dot(axis, p);
            float f = glm::dot(axis, ray.direction);

            if (std::abs(f) > 1e-6f) {
                float t1 = (e + halfExtents[i]) / f;
                float t2 = (e - halfExtents[i]) / f;

                if (t1 > t2) std::swap(t1, t2);
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                if (tMin > tMax) return false;
            }
            else {
                // Rayon parallèle au plan de cette face
                if (-e - halfExtents[i] > 0.0f || -e + halfExtents[i] < 0.0f)
                    return false;
            }
        }

        outDistance = tMin;
        outHitPoint = ray.origin + ray.direction * tMin;

        // approx normal
        glm::vec3 localHit = glm::transpose(boxRotation) * (outHitPoint - boxCenter);
        glm::vec3 localNormal(0.0f);
        for (int i = 0; i < 3; ++i) {
            if (std::abs(localHit[i] - halfExtents[i]) < 1e-3f)
                localNormal[i] = 1.0f;
            else if (std::abs(localHit[i] + halfExtents[i]) < 1e-3f)
                localNormal[i] = -1.0f;
        }
        outNormal = boxRotation * localNormal;

        return true;
    }

}*/
