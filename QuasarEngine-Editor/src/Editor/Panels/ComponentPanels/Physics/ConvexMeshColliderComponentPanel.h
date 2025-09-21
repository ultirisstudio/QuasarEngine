#pragma once

namespace QuasarEngine
{
    class Entity;

    class ConvexMeshColliderComponentPanel
    {
    public:
        ConvexMeshColliderComponentPanel() = default;
        ~ConvexMeshColliderComponentPanel() = default;

        void Render(Entity entity);
    };
}
