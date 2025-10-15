#pragma once

#include <Editor/Panels/IComponentPanel.h>

#include <string>
#include <vector>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine {

    class Entity;

    class AnimationComponentPanel : public IComponentPanel
    {
    public:
        AnimationComponentPanel() = default;
        ~AnimationComponentPanel() = default;

        void Render(Entity entity) override;
        const char* Name() const override { return ""; }

    private:
        UUID m_LastEntityID{};
        int  m_SelectedClip = -1;
        bool m_RequestReload = false;

        static std::string FormatSeconds(double sec);
    };
}