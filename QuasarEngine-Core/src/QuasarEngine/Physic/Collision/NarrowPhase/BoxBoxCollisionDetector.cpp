#include "qepch.h"
#include "BoxBoxCollisionDetector.h"

#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>
#include <QuasarEngine/Physic/Collision/ContactManifold.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/RigidBody.h>

#include <glm/gtx/matrix_transform_2d.hpp>
#include <cfloat>

namespace QuasarEngine {

    // ------------------------------------------------
    // fonction 1 : Calcul de la face incidente opposée
    // ------------------------------------------------

    void BoxBoxCollisionDetector::computeIncidentFace(glm::vec3 out[4],
        const glm::vec3& normal,
        const glm::vec3& halfExtents,
        const glm::mat3& rot,
        const glm::vec3& pos) {

        int face = 0;
        float minDot = FLT_MAX;

        // Trouver la face la plus opposée à la normale
        for (int i = 0; i < 3; ++i) {
            float d = glm::dot(normal, rot[i]);
            float absD = std::abs(d);
            if (absD < minDot) {
                minDot = absD;
                face = i;
            }
        }

        float dir = glm::sign(glm::dot(normal, rot[face]));
        glm::vec3 axisNormal = rot[face] * -dir;
        glm::vec3 center = pos + axisNormal * halfExtents[face];

        glm::vec3 tangent1 = rot[(face + 1) % 3] * halfExtents[(face + 1) % 3];
        glm::vec3 tangent2 = rot[(face + 2) % 3] * halfExtents[(face + 2) % 3];

        out[0] = center + tangent1 + tangent2;
        out[1] = center + tangent1 - tangent2;
        out[2] = center - tangent1 - tangent2;
        out[3] = center - tangent1 + tangent2;
    }

    // ------------------------------------------------
    // fonction 2 : Clipping d'une face contre un plan
    // ------------------------------------------------

    int BoxBoxCollisionDetector::clipFaceAgainstPlane(const glm::vec3& planeNormal,
        float planeDist,
        glm::vec3 inVerts[4],
        glm::vec3 outVerts[4]) {

        int numOut = 0;
        glm::vec3 prev = inVerts[3];
        float prevDist = glm::dot(planeNormal, prev) - planeDist;

        for (int i = 0; i < 4; ++i) {
            glm::vec3 curr = inVerts[i];
            float currDist = glm::dot(planeNormal, curr) - planeDist;

            if (currDist <= 0.0f) {
                if (prevDist > 0.0f) {
                    float t = prevDist / (prevDist - currDist);
                    if (numOut < 4) outVerts[numOut++] = prev + t * (curr - prev);
                }
                if (numOut < 4) outVerts[numOut++] = curr;
            }
            else if (prevDist <= 0.0f) {
                float t = prevDist / (prevDist - currDist);
                if (numOut < 4) outVerts[numOut++] = prev + t * (curr - prev);
            }

            prev = curr;
            prevDist = currDist;
        }

        return numOut;
    }

    // ------------------------------------------------
    // FONCTION 3 : Clipping final et contacts
    // ------------------------------------------------

    int BoxBoxCollisionDetector::computeContactPoints(const glm::vec3 incidentFace[4],
        const glm::vec3& refNormal,
        const glm::vec3& refCenter,
        const glm::vec3& refTangent1,
        const glm::vec3& refTangent2,
        float refHalf1,
        float refHalf2,
        glm::vec3 outContacts[4]) {

        glm::vec3 clipped1[4], clipped2[4];
        int numPoints = 4;
        std::copy(incidentFace, incidentFace + 4, clipped1);

        numPoints = clipFaceAgainstPlane(-refTangent1, refHalf1, clipped1, clipped2);
        if (numPoints < 1) return 0;

        numPoints = clipFaceAgainstPlane(refTangent1, refHalf1, clipped2, clipped1);
        if (numPoints < 1) return 0;

        numPoints = clipFaceAgainstPlane(-refTangent2, refHalf2, clipped1, clipped2);
        if (numPoints < 1) return 0;

        numPoints = clipFaceAgainstPlane(refTangent2, refHalf2, clipped2, clipped1);
        if (numPoints < 1) return 0;

        int numFinal = 0;
        for (int i = 0; i < numPoints && numFinal < 4; ++i) {
            float penetration = glm::dot(refNormal, clipped1[i] - refCenter);
            if (penetration <= 0.0f) {
                // Corriger le point sur le plan
                glm::vec3 projected = clipped1[i] - refNormal * penetration;
                outContacts[numFinal++] = projected;
            }
        }

        return numFinal;
    }

    // ------------------------------------------------
    // TestCollision : Main SAT + contact point generation
    // ------------------------------------------------

    bool BoxBoxCollisionDetector::TestCollision(Collider* A, Collider* B, ContactManifold& manifold) {
        auto* boxA = dynamic_cast<BoxShape*>(A->GetProxyShapes()[0]->GetCollisionShape());
        auto* boxB = dynamic_cast<BoxShape*>(B->GetProxyShapes()[0]->GetCollisionShape());
        if (!boxA || !boxB) return false;

        glm::vec3 posA = A->GetAttachedRigidBody()->GetPosition();
        glm::quat rotA = A->GetAttachedRigidBody()->GetOrientation();
        glm::vec3 posB = B->GetAttachedRigidBody()->GetPosition();
        glm::quat rotB = B->GetAttachedRigidBody()->GetOrientation();

        glm::mat3 rotMatA = glm::mat3_cast(rotA);
        glm::mat3 rotMatB = glm::mat3_cast(rotB);
        glm::vec3 halfA = boxA->GetHalfExtents();
        glm::vec3 halfB = boxB->GetHalfExtents();
        glm::vec3 delta = posB - posA;

        glm::mat3 R, AbsR;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                R[i][j] = glm::dot(rotMatA[i], rotMatB[j]);

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                AbsR[i][j] = std::abs(R[i][j]) + 1e-5f;

        glm::vec3 t = glm::transpose(rotMatA) * delta;

        float minPen = FLT_MAX;
        glm::vec3 bestAxis;
        int bestType = -1;

        // Axes de A
        for (int i = 0; i < 3; ++i) {
            float ra = halfA[i];
            float rb = halfB.x * AbsR[i][0] + halfB.y * AbsR[i][1] + halfB.z * AbsR[i][2];
            float overlap = ra + rb - std::abs(t[i]);
            if (overlap < 0) return false;
            if (overlap < minPen) {
                minPen = overlap;
                bestAxis = rotMatA[i] * ((t[i] < 0) ? -1.0f : 1.0f);
                bestType = 0;
            }
        }

        // Axes de B
        for (int i = 0; i < 3; ++i) {
            float ra = halfA.x * AbsR[0][i] + halfA.y * AbsR[1][i] + halfA.z * AbsR[2][i];
            float rb = halfB[i];
            float proj = t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i];
            float overlap = ra + rb - std::abs(proj);
            if (overlap < 0) return false;
            if (overlap < minPen) {
                minPen = overlap;
                bestAxis = rotMatB[i] * ((proj < 0) ? -1.0f : 1.0f);
                bestType = 1;
            }
        }

        // Axes croisés
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                glm::vec3 axis = glm::cross(rotMatA[i], rotMatB[j]);
                if (glm::length(axis) < 1e-6f) continue;
                axis = glm::normalize(axis);

                float ra = halfA.x * std::abs(glm::dot(axis, rotMatA[0])) +
                    halfA.y * std::abs(glm::dot(axis, rotMatA[1])) +
                    halfA.z * std::abs(glm::dot(axis, rotMatA[2]));
                float rb = halfB.x * std::abs(glm::dot(axis, rotMatB[0])) +
                    halfB.y * std::abs(glm::dot(axis, rotMatB[1])) +
                    halfB.z * std::abs(glm::dot(axis, rotMatB[2]));

                float dist = std::abs(glm::dot(delta, axis));
                float overlap = ra + rb - dist;

                if (overlap < 0) return false;
                if (overlap < minPen) {
                    minPen = overlap;
                    bestAxis = axis * ((glm::dot(delta, axis) < 0) ? -1.0f : 1.0f);
                    bestType = 2;
                }
            }
        }

        // --- Correction de la direction de la normale ---
        if (glm::dot(delta, bestAxis) < 0.0f) {
            bestAxis = -bestAxis;
        }

        // --- Choix de la face incidente & référence ---
        glm::vec3 incident[4], contacts[4];
        glm::vec3 tangent1, tangent2;
        float half1, half2;

        if (bestType == 0) {
            computeIncidentFace(incident, bestAxis, halfB, rotMatB, posB);
            int axis = 0;
            for (int i = 0; i < 3; ++i)
                if (std::abs(glm::dot(bestAxis, rotMatA[i])) > 0.99f) axis = i;
            tangent1 = rotMatA[(axis + 1) % 3];
            tangent2 = rotMatA[(axis + 2) % 3];
            half1 = halfA[(axis + 1) % 3];
            half2 = halfA[(axis + 2) % 3];
        }
        else {
            computeIncidentFace(incident, -bestAxis, halfA, rotMatA, posA);
            int axis = 0;
            for (int i = 0; i < 3; ++i)
                if (std::abs(glm::dot(-bestAxis, rotMatB[i])) > 0.99f) axis = i;
            tangent1 = rotMatB[(axis + 1) % 3];
            tangent2 = rotMatB[(axis + 2) % 3];
            half1 = halfB[(axis + 1) % 3];
            half2 = halfB[(axis + 2) % 3];
        }

        // --- Correction du refCenter (milieu du contact) ---
        glm::vec3 refCenter = 0.5f * (posA + posB) + bestAxis * (minPen * 0.5f);

        int nb = computeContactPoints(incident, bestAxis, refCenter, tangent1, tangent2, half1, half2, contacts);

        if (nb == 0 || minPen <= 0.0f) {
            std::cout << "[DEBUG] SAT a détecté une collision, mais aucun point généré.\n";
            std::cout << "[DEBUG] bestType=" << bestType << " penetration=" << minPen << "\n";
            std::cout << "[DEBUG] Normale : (" << bestAxis.x << ", " << bestAxis.y << ", " << bestAxis.z << ")\n";
            return false;
        }

        manifold.colliderA = A;
        manifold.colliderB = B;
        manifold.contactNormal = glm::normalize(bestAxis);
        manifold.penetrationDepth = minPen;
        manifold.contactPoints.clear();
        for (int i = 0; i < nb; ++i) {
            manifold.contactPoints.push_back(contacts[i]);
        }

        return true;
    }

}