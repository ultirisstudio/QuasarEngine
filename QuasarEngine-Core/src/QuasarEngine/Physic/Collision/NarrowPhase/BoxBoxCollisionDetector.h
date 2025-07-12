#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine {

    class Collider;
    struct ContactManifold;

    class BoxBoxCollisionDetector {
    public:
        static bool TestCollision(Collider* A, Collider* B, ContactManifold& manifold);
    private:
        static void computeIncidentFace(glm::vec3 out[4],
            const glm::vec3& normal,
            const glm::vec3& halfExtents,
            const glm::mat3& rot,
            const glm::vec3& pos);

        static int clipFaceAgainstPlane(const glm::vec3& planeNormal,
            float planeDist,
            glm::vec3 inVerts[4],
            glm::vec3 outVerts[4]);

        static int computeContactPoints(const glm::vec3 incidentFace[4],
            const glm::vec3& refNormal,
            const glm::vec3& refCenter,
            const glm::vec3& refTangent1,
            const glm::vec3& refTangent2,
            float refHalf1,
            float refHalf2,
            glm::vec3 outContacts[4]);
    };

}