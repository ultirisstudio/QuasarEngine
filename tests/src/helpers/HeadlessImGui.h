#pragma once

#include <imgui.h>
#include <imgui_internal.h>

#include <functional>
#include <utility>

namespace TestHelpers
{
    // An ImGui context without a window or a renderer, to test the code that draws ImGui windows. It plays the part
    // of the renderer for the textures of ImGui 1.92, which only needs to mark them as created.
    class HeadlessImGui
    {
    public:
        explicit HeadlessImGui(std::function<void()> draw) : m_Draw(std::move(draw)), m_Previous(ImGui::GetCurrentContext()), m_Context(ImGui::CreateContext())
        {
            ImGui::SetCurrentContext(m_Context);
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(1600.0f, 900.0f);
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = nullptr;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            Frame();
        }

        ~HeadlessImGui()
        {
            ImGui::DestroyContext(m_Context);
            ImGui::SetCurrentContext(m_Previous);
        }

        HeadlessImGui(const HeadlessImGui&) = delete;
        HeadlessImGui& operator=(const HeadlessImGui&) = delete;
        HeadlessImGui(HeadlessImGui&&) = delete;
        HeadlessImGui& operator=(HeadlessImGui&&) = delete;

        void Frame() const
        {
            ImGui::NewFrame();
            m_Draw();
            ImGui::Render();
            for (ImTextureData* texture : ImGui::GetPlatformIO().Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetTexID(1);
                    texture->SetStatus(ImTextureStatus_OK);
                }
                else if (texture->Status == ImTextureStatus_WantDestroy)
                {
                    texture->SetTexID(ImTextureID_Invalid);
                    texture->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }

        void Type(const char* text) const
        {
            ImGui::GetIO().AddInputCharactersUTF8(text);
            Frame();
        }

        void Press(ImGuiKeyChord chord) const
        {
            SetKeys(chord, true);
            Frame();
            SetKeys(chord, false);
            Frame();
        }

        static bool IsPopupOpen() { return ImGui::GetTopMostPopupModal() != nullptr; }

    private:
        static void SetKeys(ImGuiKeyChord chord, bool down)
        {
            ImGuiIO& io = ImGui::GetIO();
            for (const ImGuiKey modifier : {ImGuiMod_Ctrl, ImGuiMod_Shift, ImGuiMod_Alt, ImGuiMod_Super})
            {
                if ((chord & modifier) != 0)
                {
                    io.AddKeyEvent(modifier, down);
                }
            }
            io.AddKeyEvent(static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_), down);
        }

        std::function<void()> m_Draw;
        ImGuiContext* m_Previous;
        ImGuiContext* m_Context;
    };
}
