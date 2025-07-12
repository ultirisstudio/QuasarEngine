#include "qepch.h"
#include "CollisionDetection.h"
#include <QuasarEngine/Physic/Collision/Collider.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Physic/RigidBody.h>
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#include <QuasarEngine/Physic/Collision/NarrowPhase/BoxBoxCollisionDetector.h>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <QuasarEngine/Physic/Collision/ContactManifold.h>
#include <QuasarEngine/Physic/Collision/NarrowPhase/SphereVsSphereCollisionDetector.h>
#include <QuasarEngine/Physic/Shape/SphereShape.h>
#include <iostream>

namespace QuasarEngine {

    std::vector<Collider*> CollisionDetection::s_Colliders;
    CollisionFunc CollisionDetection::s_DispatchMatrix[3][3];

    void CollisionDetection::Init() {
        // Initialise toutes les cases à null
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                s_DispatchMatrix[i][j] = nullptr;

        // Box vs Box
        s_DispatchMatrix[(int)CollisionShapeType::Box][(int)CollisionShapeType::Box] =
            &BoxBoxCollisionDetector::TestCollision;

        // Sphere vs Sphere
        s_DispatchMatrix[(int)CollisionShapeType::Sphere][(int)CollisionShapeType::Sphere] =
            &SphereVsSphereCollisionDetector::TestCollision;

    }


    void CollisionDetection::RegisterCollider(Collider* collider) {
        if (collider && std::find(s_Colliders.begin(), s_Colliders.end(), collider) == s_Colliders.end()) {
            s_Colliders.push_back(collider);
        }
    }

    void CollisionDetection::UnregisterCollider(Collider* collider) {
        s_Colliders.erase(std::remove(s_Colliders.begin(), s_Colliders.end(), collider), s_Colliders.end());
    }

    bool CollisionDetection::CheckCollision(Collider* A, Collider* B, ContactManifold& manifold) {
        if (!A || !B) return false;

        const auto& shapesA = A->GetProxyShapes();
        const auto& shapesB = B->GetProxyShapes();

        if (shapesA.empty() || shapesB.empty()) return false;

        CollisionShape* shapeA = shapesA[0]->GetCollisionShape();
        CollisionShape* shapeB = shapesB[0]->GetCollisionShape();

        CollisionShapeType typeA = shapeA->GetType();
        CollisionShapeType typeB = shapeB->GetType();

        auto func = s_DispatchMatrix[(int)typeA][(int)typeB];
        if (func) {
            return func(A, B, manifold);
        }

        std::cout << "[CollisionDetection] Type de collision non pris en charge : "
            << static_cast<int>(typeA) << " vs " << static_cast<int>(typeB) << std::endl;

        return false;
    }

    void CollisionDetection::CheckAllCollisions(std::vector<ContactManifold>& manifolds) {
        for (size_t i = 0; i < s_Colliders.size(); ++i) {
            for (size_t j = i + 1; j < s_Colliders.size(); ++j) {
                Collider* A = s_Colliders[i];
                Collider* B = s_Colliders[j];

                ContactManifold manifold;
                if (CheckCollision(A, B, manifold)) {
                    manifolds.push_back(manifold);
                }
            }
        }
    }

}