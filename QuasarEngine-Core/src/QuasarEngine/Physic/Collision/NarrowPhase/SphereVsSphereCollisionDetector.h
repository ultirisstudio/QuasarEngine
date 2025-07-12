#pragma once

#include "QuasarEngine/Physic/Collision/Collider.h"
#include "QuasarEngine/Physic/Collision/ContactManifold.h"

namespace QuasarEngine {

    class SphereVsSphereCollisionDetector {
    public:
        static bool TestCollision(Collider* colliderA, Collider* colliderB, ContactManifold& manifold);
    };

}