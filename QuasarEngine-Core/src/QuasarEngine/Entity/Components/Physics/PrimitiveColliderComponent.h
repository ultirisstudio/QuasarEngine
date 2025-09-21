#pragma once

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
    class PrimitiveColliderComponent : public Component
    {
    public:
        float mass = 1.0f;
        float friction = 0.5f;
        float bounciness = 0.2f;

        PrimitiveColliderComponent() = default;
        virtual ~PrimitiveColliderComponent() = default;

        virtual void Init() = 0;
        virtual void UpdateColliderMaterial() = 0;
        virtual void UpdateColliderSize() = 0;
    };
}