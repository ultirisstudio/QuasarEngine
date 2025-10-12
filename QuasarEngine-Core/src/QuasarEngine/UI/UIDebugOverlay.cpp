#include "qepch.h"
#include "UIDebugOverlay.h"

#include <GLFW/glfw3.h>

namespace QuasarEngine {

    void UIDebugOverlay::Post(Severity sev, std::string msg, float ttlSeconds) {
        m_Messages.push_back(Msg{ std::move(msg), sev, NowSeconds(), ttlSeconds });
    }

    void UIDebugOverlay::Clear() { m_Messages.clear(); }

    float UIDebugOverlay::NowSeconds() const {
        return static_cast<float>(glfwGetTime());
    }

    ImVec4 UIDebugOverlay::ColorFor(Severity s) const {
        switch (s) {
        case Severity::Info:  return ImVec4(0.8f, 0.9f, 1.0f, 1.0f);
        case Severity::Warn:  return ImVec4(1.0f, 0.85f, 0.4f, 1.0f);
        case Severity::Error: return ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
        }
        return ImVec4(1, 1, 1, 1);
    }

    void UIDebugOverlay::GC(float now) {
        m_Messages.erase(
            std::remove_if(m_Messages.begin(), m_Messages.end(),
                [&](const Msg& m) { return (now - m.t0) > m.ttl; }),
            m_Messages.end()
        );
    }

    void UIDebugOverlay::DrawImGui(const ImVec2& topLeftAnchor) {
        const float now = NowSeconds();
        GC(now);
        if (m_Messages.empty())
            return;

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::SetNextWindowPos(ImVec2(topLeftAnchor.x + 8.0f, topLeftAnchor.y + 8.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);

        ImGui::Begin("UI Debug Overlay", nullptr, flags);
        for (const auto& m : m_Messages) {
            ImGui::PushStyleColor(ImGuiCol_Text, ColorFor(m.sev));
            ImGui::TextUnformatted(m.text.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }
}