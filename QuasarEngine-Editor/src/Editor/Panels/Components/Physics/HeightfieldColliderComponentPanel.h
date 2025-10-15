#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class HeightfieldColliderComponentPanel : public IComponentPanel
    {
    public:
        HeightfieldColliderComponentPanel() = default;
        ~HeightfieldColliderComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }
    };
}
