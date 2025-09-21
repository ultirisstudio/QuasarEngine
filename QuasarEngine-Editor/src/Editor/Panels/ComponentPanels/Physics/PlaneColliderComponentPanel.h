#pragma once

namespace QuasarEngine
{
    class Entity;

    class PlaneColliderComponentPanel
    {
    public:
        PlaneColliderComponentPanel() = default;
        ~PlaneColliderComponentPanel() = default;

        void Render(Entity entity);
    };
}
