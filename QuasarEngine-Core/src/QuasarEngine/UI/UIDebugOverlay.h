#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include <imgui/imgui.h>

namespace QuasarEngine {

    class UIDebugOverlay {
    public:
        enum class Severity { Info, Warn, Error };

        static UIDebugOverlay& Instance() { static UIDebugOverlay s; return s; }

        void Post(Severity sev, std::string msg, float ttlSeconds = 5.0f);

        void DrawImGui(const ImVec2& topLeftAnchor);

        void Clear();

    private:
        struct Msg {
            std::string text;
            Severity sev;
            float t0;
            float ttl;
        };

        std::vector<Msg> m_Messages;

        UIDebugOverlay() = default;
        ~UIDebugOverlay() = default;

        float NowSeconds() const;
        void GC(float now);
        ImVec4 ColorFor(Severity s) const;
    };

}