#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>

namespace QuasarEngine {

    class RigidBody;

    class Collider {
    public:
        Collider(RigidBody* body);
        ~Collider() = default;

        ProxyShape* AddShape(std::unique_ptr<CollisionShape> shape);

        RigidBody* GetAttachedRigidBody() const { return rigidBody; }

        const std::vector<std::unique_ptr<ProxyShape>>& GetProxyShapes() const { return proxyShapes; }
        std::vector<std::unique_ptr<ProxyShape>>& GetProxyShapes();
        void ClearProxyShapes() { proxyShapes.clear(); }
    private:
        RigidBody* rigidBody;
        std::vector<std::unique_ptr<ProxyShape>> proxyShapes;
    };

}