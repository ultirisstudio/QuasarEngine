#pragma once

namespace QuasarEngine
{
    class Entity;

    class TriangleMeshColliderComponentPanel
    {
    public:
        TriangleMeshColliderComponentPanel() = default;
        ~TriangleMeshColliderComponentPanel() = default;

        void Render(Entity entity);
    };
}
