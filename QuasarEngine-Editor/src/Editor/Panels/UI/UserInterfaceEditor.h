#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace QuasarEngine
{
    class UserInterfaceEditor
    {
    public:
        enum class UIType : uint8_t
        {
            Button,
            Text,
            Checkbox,
            ProgressBar,
            Separator,
            Image,
            Slider,
            InputText
        };


        struct UIFlags
        {
            bool visible = true;
            bool disabled = false;
            bool sameLine = false;
        };

        struct UIRect
        {
            float x = 20.0f;
            float y = 20.0f;
            float w = 120.0f;
            float h = 32.0f;
        };

        enum class AlignH : uint8_t { Left, Center, Right };
        enum class AlignV : uint8_t { Top, Middle, Bottom };

        struct UIElement
        {
            uint32_t    id = 0;
            UIType      type = UIType::Button;
            UIRect      rect;
            UIFlags     flags;

            char        label[128] = "Button";
            bool        checked = false;
            float       fraction = 0.5f;
            char        overlay[64] = "";

            AlignH      alignH = AlignH::Center;
            AlignV      alignV = AlignV::Middle;

            float       fontPx = 16.0f;

            char        imagePath[260] = "";
            char        imageId[260] = "";
            bool        imageKeepAspect = true;
            float       imageTint[4] = { 1,1,1,1 };

            float       sliderMin = 0.0f;
            float       sliderMax = 1.0f;
            float       sliderValue = 0.5f;
            char        sliderFormat[16] = "%.2f";

            char        inputBuffer[256] = "";
            uint32_t    inputMaxLen = 256;
            bool        inputPassword = false;
            bool        inputMultiline = false;
        };

        UserInterfaceEditor();
        ~UserInterfaceEditor() = default;

        void OnImGuiRender(const char* windowName = "User Interface Editor");
        void Update() {}

        bool New();
        bool LoadFromFile(const char* path);
        bool SaveToFile(const char* path) const;

        void SetDesignResolution(uint32_t w, uint32_t h);

    private:
        mutable std::mutex m_Mutex;
        std::vector<UIElement> m_Elements;
        uint32_t m_NextId = 1;

        int      m_SelectedIndex = -1;

        ImVec2   m_CanvasPos{ 0,0 };
        ImVec2   m_CanvasSize{ 0,0 };
        ImVec2   m_Pan{ 20,20 };
        float    m_Zoom = 1.0f;
        bool     m_ShowGrid = true;
        bool     m_SnapToGrid = true;
        float    m_GridStep = 35.0f;

        bool     m_Dragging = false;
        bool     m_Resizing = false;
        ImVec2   m_DragOffset{ 0,0 };
        int      m_ResizeHandle = -1;
        const float m_HandleSize = 8.0f;

        uint32_t m_DesignW = 1920;
        uint32_t m_DesignH = 1080;

        struct Snapshot
        {
            std::vector<UIElement> elements;
            int selected = -1;
            uint32_t nextId = 1;
        };
        std::vector<Snapshot> m_Undo;
        std::vector<Snapshot> m_Redo;

        std::string m_OpenedPath;

    private:
        static inline float Saturate(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
        static inline float Snap(float v, float step) { return step > 1e-6f ? std::round(v / step) * step : v; }

        void DrawMainToolbar();
        void DrawHierarchyPanel(float width, float height);
        void DrawCanvasPanel(float width, float height);
        void DrawPropertiesPanel(float width, float height);
        void DrawPreviewPanel(float width, float height);

        void DrawGrid(ImDrawList* dl, ImVec2 origin, ImVec2 size, float step) const;
        void DrawElement(ImDrawList* dl, const UIElement& e, bool selected) const;
        void DrawResizeHandles(ImDrawList* dl, const UIElement& e) const;
        bool HitTest(const UIElement& e, ImVec2 p) const;
        int  HitTestHandle(const UIElement& e, ImVec2 p) const;

        int  AddElement(UIType type, ImVec2 canvasPt);
        void DeleteSelected();
        void DuplicateSelected();
        void BringToFront();
        void SendToBack();

        void Select(int idx);
        void StartDrag(ImVec2 mouseCanvas, const UIElement& e);
        void StartResize(int handleIndex);
        void ApplyDrag(ImVec2 mouseCanvas);
        void ApplyResize(ImVec2 mouseCanvas);

        void PushUndo(const char* reason = nullptr);
        void DoUndo();
        void DoRedo();

        bool Serialize(const char* path) const;
        bool Deserialize(const char* path);

        ImVec2 ScreenToCanvas(ImVec2 screen) const;
        ImVec2 CanvasToScreen(ImVec2 canvas) const;
        void   ClampRect(UIRect& r) const;
        static const char* TypeToString(UIType t);
        static UIType      StringToType(const char* s);

        ImVec2 ComputeAlignedTextPos(const ImVec2& rectMin, const ImVec2& rectSize, const char* text, AlignH ah, AlignV av, float fontScale) const;

        struct FontScaleScope {
            float prev = 1.f;
            FontScaleScope(float s) {
                prev = ImGui::GetCurrentWindow()->FontWindowScale;
                ImGui::SetWindowFontScale(s);
            }
            ~FontScaleScope() { ImGui::SetWindowFontScale(prev); }
        };

        void RenderRuntimePreview(float scale, ImVec2 offset) const;
    };
}