#pragma once

namespace QuasarEngine
{
    class Entity;

    class HeightfieldColliderComponentPanel
    {
    public:
        HeightfieldColliderComponentPanel() = default;
        ~HeightfieldColliderComponentPanel() = default;

        void Render(Entity entity);
    };
}
