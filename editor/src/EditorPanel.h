#pragma once

#include <quasar/time/Timestep.h>

#include <imgui.h>

#include <string>

namespace Quasar
{
    class Event;
}

namespace Quasar::Editor
{
    struct EditorContext;

    class EditorPanel
    {
    public:
        virtual ~EditorPanel() = default;

        EditorPanel(const EditorPanel&) = delete;
        EditorPanel& operator=(const EditorPanel&) = delete;
        EditorPanel(EditorPanel&&) = delete;
        EditorPanel& operator=(EditorPanel&&) = delete;

        void Draw();

        virtual void OnUpdate([[maybe_unused]] Timestep timestep) {}
        virtual void OnRender() {}
        virtual void OnEvent([[maybe_unused]] Event& event) {}

        // The key of the title in the texts of the editor, which also identifies the window
        const std::string& GetKey() const { return m_Key; }

        // The title in the language of the editor, followed by ### and the key
        std::string GetLabel() const;
        bool IsOpen() const { return m_Open; }
        void SetOpen(bool open) { m_Open = open; }
        bool IsFocused() const { return m_Focused; }
        bool IsHovered() const { return m_Hovered; }

    protected:
        EditorPanel(std::string key, EditorContext& context, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

        virtual void OnImGuiRender() = 0;

        EditorContext& GetContext() const { return *m_Context; }
        void SetInitialSize(ImVec2 size) { m_InitialSize = size; }

    private:
        std::string m_Key;
        EditorContext* m_Context;
        ImGuiWindowFlags m_Flags;
        ImVec2 m_InitialSize{0.0f, 0.0f};
        bool m_Open = true;
        bool m_Focused = false;
        bool m_Hovered = false;
    };
}
