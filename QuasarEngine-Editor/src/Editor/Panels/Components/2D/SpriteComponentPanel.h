#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class SpriteComponentPanel : public IComponentPanel
    {
    public:
        SpriteComponentPanel();
        ~SpriteComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }

    private:
        bool s_ShowGrid;
        int s_GridX;
        int s_GridY;
    };
}
