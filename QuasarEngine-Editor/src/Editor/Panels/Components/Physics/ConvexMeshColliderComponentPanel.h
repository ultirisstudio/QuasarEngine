#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class ConvexMeshColliderComponentPanel : public IComponentPanel
    {
    public:
        ConvexMeshColliderComponentPanel() = default;
        ~ConvexMeshColliderComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }
    };
}
