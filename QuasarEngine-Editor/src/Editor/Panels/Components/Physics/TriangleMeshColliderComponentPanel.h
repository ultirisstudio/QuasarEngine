#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class TriangleMeshColliderComponentPanel : public IComponentPanel
    {
    public:
        TriangleMeshColliderComponentPanel() = default;
        ~TriangleMeshColliderComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }
    };
}
