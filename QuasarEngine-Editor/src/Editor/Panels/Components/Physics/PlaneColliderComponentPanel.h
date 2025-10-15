#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class PlaneColliderComponentPanel : public IComponentPanel
    {
    public:
        PlaneColliderComponentPanel() = default;
        ~PlaneColliderComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }
    };
}
