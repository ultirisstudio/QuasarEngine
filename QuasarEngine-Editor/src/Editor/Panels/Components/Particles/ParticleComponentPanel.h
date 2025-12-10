#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
    class Entity;

    class ParticleComponentPanel : public IComponentPanel
    {
    public:
        ParticleComponentPanel() = default;
        ~ParticleComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return "Particles"; }
    };
}
