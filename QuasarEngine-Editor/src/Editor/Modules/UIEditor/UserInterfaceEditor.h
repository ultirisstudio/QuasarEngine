#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <QuasarEngine/UI/UIElement.h>
#include <QuasarEngine/UI/UIContainer.h>
#include <QuasarEngine/UI/UIButton.h>
#include <QuasarEngine/UI/UIText.h>
#include <QuasarEngine/UI/UITextInput.h>
#include <QuasarEngine/UI/UICheckbox.h>
#include <QuasarEngine/UI/UISlider.h>
#include <QuasarEngine/UI/UIProgressBar.h>
#include <QuasarEngine/UI/UITabBar.h>
#include <QuasarEngine/UI/UIMenu.h>
#include <QuasarEngine/UI/UITooltipLayer.h>
#include <QuasarEngine/UI/UIImage.h>
#include <QuasarEngine/UI/UISeparator.h>

#include <QuasarEngine/UI/UISerialize.h>

#include <Editor/Modules/IEditorModule.h>
#include <Editor/EditorCanvas.h>
#include <Editor/UndoStack.h>

namespace QuasarEngine
{
    class UserInterfaceEditor : public IEditorModule
    {
    public:
        UserInterfaceEditor(EditorContext& context);
        ~UserInterfaceEditor() override;

        void Update(double dt) override;
		void Render() override;
        void RenderUI() override;

        bool New();
        bool LoadFromFile(const char* path);
        bool SaveToFile(const char* path) const;

        void SetDesignResolution(uint32_t w, uint32_t h);

    private:
        mutable std::mutex m_Mutex;
        std::shared_ptr<UIElement> m_Root;
        std::weak_ptr<UIElement> m_Selected;
        std::string m_OpenedPath;

        EditorCanvas m_Canvas;
		bool m_SnapToGrid = true;

        bool m_Dragging = false;
        bool m_Resizing = false;
        ImVec2 m_DragOffset{ 0,0 };
        int m_ResizeHandle = -1;
        const float m_HandleSize = 8.0f;

        uint32_t m_DesignW = 1920;
        uint32_t m_DesignH = 1080;

        UndoStack<std::vector<uint8_t>> m_UndoStack{ 64 };

        std::string m_SelectedIdCache;

        static inline float Saturate(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
        static inline float Snap(float v, float step) { return step > 1e-6f ? std::round(v / step) * step : v; }

        void DrawMainToolbar();
        void DrawHierarchyPanel(float width, float height);
        void DrawCanvasPanel(float width, float height);
        void DrawPropertiesPanel(float width, float height);
        void DrawPreviewPanel(float width, float height);

        void DrawElementBox(ImDrawList* dl, UIElement* e, bool selected) const;
        void DrawResizeHandles(ImDrawList* dl, UIElement* e) const;

        bool HitTest(UIElement* e, ImVec2 canvasPt) const;
        int  HitTestHandle(UIElement* e, ImVec2 screen) const;

        std::shared_ptr<UIElement> CreateElement(UISerType type, const glm::vec2& pos);
        void DeleteSelected();
        void DuplicateSelected();
        void BringToFront();
        void SendToBack();

        void Select(const std::shared_ptr<UIElement>& e);
        void StartDrag(ImVec2 mouseCanvas, UIElement* e);
        void StartResize(int handleIndex);
        void ApplyDrag(ImVec2 mouseCanvas);
        void ApplyResize(ImVec2 mouseCanvas);

        void PushUndo(const char* reason = nullptr);
        void DoUndo();
        void DoRedo();

        bool SerializeToBuffer(const std::shared_ptr<UIElement>& root, std::vector<uint8_t>& out) const;
        std::shared_ptr<UIElement> DeserializeFromBuffer(const uint8_t* data, size_t size) const;

        void ClampRect(Rect& r) const;

        void FlattenZOrder(const std::shared_ptr<UIElement>& root, std::vector<UIElement*>& out) const;
        std::shared_ptr<UIElement> FindById(const std::string& id) const;
        std::shared_ptr<UIElement> ParentOf(const std::shared_ptr<UIElement>& node);
        std::shared_ptr<const UIElement> ParentOf(const std::shared_ptr<const UIElement>& node) const;

        bool Reparent(const std::shared_ptr<UIElement>& node, const std::shared_ptr<UIElement>& newParent, int insertIndex = -1);
        bool ReorderWithinParent(const std::shared_ptr<UIElement>& node, int newIndex);

        static const char* SerTypeToString(UISerType t);
        static UISerType StringToSerType(const char* s);
        static ImU32 ToImColor(const UIColor& c);
        static std::string MakeUniqueId(const std::string& base, const std::shared_ptr<UIElement>& root);

        void DrawCommonProperties(UIElement& e, bool& anyChanged);
        void DrawTypedProperties(UIElement& e, bool& anyChanged);

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
