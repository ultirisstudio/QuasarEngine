#pragma once

#include <vector>
#include <QuasarEngine/Physic/Collision/NarrowPhase/SphereVsSphereCollisionDetector.h>
#include <QuasarEngine/Physic/Shape/SphereShape.h>

namespace QuasarEngine {

    class Collider;
    struct ContactManifold;
    using CollisionFunc = bool(*)(Collider*, Collider*, ContactManifold&);
    class CollisionDetection {
    public:

        static void Init();

        static void RegisterCollider(Collider* collider);
        static void UnregisterCollider(Collider* collider);

        static bool CheckCollision(Collider* A, Collider* B, ContactManifold& manifold);

        // Détection brute-force entre tous les colliders enregistrés
        static void CheckAllCollisions(std::vector<ContactManifold>& manifolds);

    private:
        static std::vector<Collider*> s_Colliders;
        static CollisionFunc s_DispatchMatrix[3][3];
    };

}