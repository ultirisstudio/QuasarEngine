#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace QuasarEngine {

    class Collider;

    struct ContactPoint {
        glm::vec3 contactPoint;
        glm::vec3 normal;
        float penetrationDepth;
    };

    struct ContactManifold {
        Collider* colliderA = nullptr;
        Collider* colliderB = nullptr;
        glm::vec3 contactNormal = glm::vec3(0.0f);
        float penetrationDepth = 0.0f;
        std::vector<glm::vec3> contactPoints;
        std::vector<ContactPoint> contacts;
    };

}