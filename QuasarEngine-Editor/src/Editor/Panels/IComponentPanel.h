#pragma once

namespace QuasarEngine
{
    class Entity;
    class Scene;

    class IComponentPanel
    {
    public:
        virtual ~IComponentPanel() = default;
        virtual const char* Name() const = 0;
        virtual void Render(Entity entity) = 0;
    };
}