#include "qepch.h"
#include "Collider.h"
#include "QuasarEngine/Physic/Shape/ProxyShape.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include "QuasarEngine/Physic/Shape/CollisionShape.h"
#include <memory>
#include <QuasarEngine/Physic/Shape/BoxShape.h>

namespace QuasarEngine {

    Collider::Collider(RigidBody* body)
        : rigidBody(body)
    {
    }

    ProxyShape* Collider::AddShape(std::unique_ptr<CollisionShape> shape)
    {
        auto proxyShape = std::make_unique<ProxyShape>(rigidBody, this, std::move(shape));

        ProxyShape* rawPtr = proxyShape.get();
        proxyShapes.push_back(std::move(proxyShape));

        return rawPtr;
    }

    std::vector<std::unique_ptr<ProxyShape>>& Collider::GetProxyShapes() {
        return proxyShapes;
    }

}