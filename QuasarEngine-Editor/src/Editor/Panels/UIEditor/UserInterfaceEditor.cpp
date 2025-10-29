#include "UserInterfaceEditor.h"

#include <fstream>
#include <queue>
#include <unordered_map>
#include <cstring>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    static glm::vec2 ToGlm(ImVec2 v) { return { v.x, v.y }; }
    static ImVec2    ToIm(glm::vec2 v) { return { v.x, v.y }; }

    ImU32 UserInterfaceEditor::ToImColor(const UIColor& c) {
        auto clamp = [](float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
        return IM_COL32((int)(clamp(c.r) * 255.f), (int)(clamp(c.g) * 255.f), (int)(clamp(c.b) * 255.f), (int)(clamp(c.a) * 255.f));
    }

    const char* UserInterfaceEditor::SerTypeToString(UISerType t) {
        switch (t) {
        case UISerType::Element: return "Element";
        case UISerType::Container: return "Container";
        case UISerType::Button: return "Button";
        case UISerType::Text: return "Text";
        case UISerType::Checkbox: return "Checkbox";
        case UISerType::ProgressBar: return "ProgressBar";
        case UISerType::Slider: return "Slider";
        case UISerType::InputText: return "InputText";
        case UISerType::Image: return "Image";
        case UISerType::Separator: return "Separator";
        case UISerType::TabBar: return "TabBar";
        case UISerType::Tabs: return "Tabs";
        case UISerType::Menu: return "Menu";
        case UISerType::TooltipLayer: return "TooltipLayer";
        }
        return "Unknown";
    }

    UISerType UserInterfaceEditor::StringToSerType(const char* s) {
        if (!s) return UISerType::Element;
        if (strcmp(s, "Container") == 0) return UISerType::Container;
        if (strcmp(s, "Button") == 0) return UISerType::Button;
        if (strcmp(s, "Text") == 0) return UISerType::Text;
        if (strcmp(s, "Checkbox") == 0) return UISerType::Checkbox;
        if (strcmp(s, "ProgressBar") == 0) return UISerType::ProgressBar;
        if (strcmp(s, "Slider") == 0) return UISerType::Slider;
        if (strcmp(s, "InputText") == 0) return UISerType::InputText;
        if (strcmp(s, "Image") == 0) return UISerType::Image;
        if (strcmp(s, "Separator") == 0) return UISerType::Separator;
        if (strcmp(s, "TabBar") == 0) return UISerType::TabBar;
        if (strcmp(s, "Tabs") == 0) return UISerType::Tabs;
        if (strcmp(s, "Menu") == 0) return UISerType::Menu;
        if (strcmp(s, "TooltipLayer") == 0) return UISerType::TooltipLayer;
        return UISerType::Element;
    }

    std::string UserInterfaceEditor::MakeUniqueId(const std::string& base, const std::shared_ptr<UIElement>& root) {
        std::unordered_set<std::string> ids;
        std::vector<UIElement*> flat;
        std::function<void(UIElement*)> dfs = [&](UIElement* n) {
            if (!n) return;
            ids.insert(n->Id());
            for (auto& c : n->Children()) dfs(c.get());
            };
        if (root) dfs(root.get());
        if (!ids.count(base)) return base;
        for (int k = 1; k < 100000; k++) {
            std::string v = base + "_" + std::to_string(k);
            if (!ids.count(v)) return v;
        }
        return base + "_x";
    }

    UserInterfaceEditor::UserInterfaceEditor() { New(); }

    bool UserInterfaceEditor::New() {
        std::lock_guard<std::mutex> lk(m_Mutex);
        
        m_Root = std::make_shared<UIContainer>("root");
        m_Root->Transform().pos = { 24,24 };
        m_Root->Transform().size = { std::max(220.f, (float)m_DesignW - 48.f), std::max(28.f, (float)m_DesignH - 48.f) };

        auto text = std::make_shared<UIText>("title");
        text->Transform().pos = { 24, 24 };
        text->Transform().size = { 320, 28 };
        text->text = "Nouvelle interface";
        m_Root->AddChild(text);

        m_OpenedPath.clear();
        m_Selected.reset();
        m_SelectedIdCache.clear();
        m_Undo.clear();
        m_Redo.clear();
        PushUndo("New");
        return true;
    }

    bool UserInterfaceEditor::LoadFromFile(const char* path) {
        if (!path) return false;
        std::lock_guard<std::mutex> lk(m_Mutex);
        auto root = UILoadFromFile(path);
        if (!root) return false;
        m_Root = std::move(root);
        m_OpenedPath = path;
        m_Selected.reset();
        m_SelectedIdCache.clear();
        m_Undo.clear(); m_Redo.clear();
        PushUndo("Load");
        return true;
    }

    bool UserInterfaceEditor::SaveToFile(const char* path) const {
        if (!path) return false;
        std::lock_guard<std::mutex> lk(m_Mutex);
        UISaveOptions opt{};
        const bool ok = UISaveToFile(m_Root, path, opt);
        return ok;
    }

    void UserInterfaceEditor::SetDesignResolution(uint32_t w, uint32_t h) {
        m_DesignW = (w == 0 ? 1u : w);
        m_DesignH = (h == 0 ? 1u : h);
    }

    void UserInterfaceEditor::OnImGuiRender(const char* windowName) {
        ImGui::SetNextWindowSize(ImVec2(1280, 760), ImGuiCond_Once);
        ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        DrawMainToolbar();
        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float leftRatio = 0.25f;
        static float rightRatio = 0.30f;
        float minLeft = 240.f, minRight = 320.f;
        float leftW = ImMax(avail.x * leftRatio, minLeft);
        float rightW = ImMax(avail.x * rightRatio, minRight);
        float centerW = ImMax(avail.x - leftW - rightW - 12.f, 320.f);

        ImGui::BeginChild("##Hierarchy", ImVec2(leftW, 0), true);
        DrawHierarchyPanel(leftW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##split1", ImVec2(6, avail.y));
        if (ImGui::IsItemActive()) {
            leftRatio += ImGui::GetIO().MouseDelta.x / avail.x;
            leftRatio = std::clamp(leftRatio, 0.12f, 0.5f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Canvas", ImVec2(centerW, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        DrawCanvasPanel(centerW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##split2", ImVec2(6, avail.y));
        if (ImGui::IsItemActive()) {
            rightRatio -= ImGui::GetIO().MouseDelta.x / avail.x;
            rightRatio = std::clamp(rightRatio, 0.18f, 0.6f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Right", ImVec2(rightW, 0), true);
        {
            ImVec2 rAvail = ImGui::GetContentRegionAvail();
            static float propRatio = 0.58f;
            float propH = ImMax(rAvail.y * propRatio, 220.f);

            ImGui::BeginChild("##Props", ImVec2(-1, propH), true);
            DrawPropertiesPanel(rightW, propH);
            ImGui::EndChild();

            ImGui::InvisibleButton("##splitR", ImVec2(-1, 8));
            if (ImGui::IsItemActive()) {
                propRatio += ImGui::GetIO().MouseDelta.y / rAvail.y;
                propRatio = std::clamp(propRatio, 0.3f, 0.85f);
            }

            ImGui::BeginChild("##Preview", ImVec2(-1, -1), true);
            DrawPreviewPanel(rightW, ImGui::GetContentRegionAvail().y);
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void UserInterfaceEditor::DrawMainToolbar() {
        if (ImGui::Button("Nouveau")) { New(); }
        ImGui::SameLine();
        if (ImGui::Button("Ouvrir...")) {
            //LoadFromFile("ui.qui");
        }
        ImGui::SameLine();
        if (ImGui::Button("Sauver")) {
            if (m_OpenedPath.empty()) {
                //SaveToFile("ui.qui");
            }
            else {
                //SaveToFile(m_OpenedPath.c_str());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Sauver sous...")) {
            //SaveToFile("ui.qui");
        }
        ImGui::SameLine(0, 24);
        if (ImGui::Button("Undo")) {
            //DoUndo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo")) {
            //DoRedo();
        }

        ImGui::SameLine(0, 24);
        ImGui::Text("Design: %ux%u", m_DesignW, m_DesignH);

        ImGui::SameLine(0, 24);
        ImGui::Checkbox("Grille", &m_ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &m_SnapToGrid);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Pas", &m_GridStep, 0.5f, 2.f, 64.f, "%.0f");

        ImGui::SameLine(0, 24);
        ImGui::SetNextItemWidth(90);
        ImGui::SliderFloat("Zoom", &m_Zoom, 0.5f, 3.0f, "%.2f");

        ImGui::SameLine(0, 24);
        if (ImGui::Button("Dupliquer")) DuplicateSelected();
        ImGui::SameLine();
        if (ImGui::Button("Supprimer")) DeleteSelected();
        ImGui::SameLine();
        if (ImGui::Button("Avant-plan")) BringToFront();
        ImGui::SameLine();
        if (ImGui::Button("Arriere-plan")) SendToBack();
    }

    static bool TreeNodeExFlat(const char* label, bool selected) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (selected ? ImGuiTreeNodeFlags_Selected : 0);
        ImGui::TreeNodeEx(label, flags);
        bool clicked = ImGui::IsItemClicked();
        return clicked;
    }

    void UserInterfaceEditor::FlattenZOrder(const std::shared_ptr<UIElement>& root, std::vector<UIElement*>& out) const {
        if (!root) return;
        out.push_back(root.get());
        for (auto& c : root->Children()) FlattenZOrder(c, out);
    }

    std::shared_ptr<UIElement> UserInterfaceEditor::FindById(const std::string& id) const {
        if (!m_Root) return {};
        if (m_Root->Id() == id) return m_Root;
        std::function<std::shared_ptr<UIElement>(std::shared_ptr<UIElement>)> dfs;
        dfs = [&](std::shared_ptr<UIElement> n)->std::shared_ptr<UIElement> {
            for (auto& c : n->Children()) {
                if (c->Id() == id) return c;
                auto r = dfs(c);
                if (r) return r;
            }
            return {};
            };
        return dfs(m_Root);
    }

    std::shared_ptr<UIElement> UserInterfaceEditor::ParentOf(const std::shared_ptr<UIElement>& node)
    {
        if (!node) return {};
        std::function<std::shared_ptr<UIElement>(std::shared_ptr<UIElement>)> findp;
        findp = [&](std::shared_ptr<UIElement> cur)->std::shared_ptr<UIElement> {
            for (auto& c : cur->Children()) {
                if (c.get() == node.get()) return cur;
                auto r = findp(c);
                if (r) return r;
            }
            return {};
            };
        if (m_Root.get() == node.get()) return {};
        return findp(m_Root);
    }

    std::shared_ptr<const UIElement> UserInterfaceEditor::ParentOf(const std::shared_ptr<const UIElement>& node) const {
        if (!node) return {};
        std::function<std::shared_ptr<UIElement>(std::shared_ptr<UIElement>)> findp;
        findp = [&](std::shared_ptr<UIElement> cur)->std::shared_ptr<UIElement> {
            for (auto& c : cur->Children()) {
                if (c.get() == node.get()) return cur;
                auto r = findp(c);
                if (r) return r;
            }
            return {};
            };
        if (m_Root.get() == node.get()) return {};
        return findp(m_Root);
    }

    bool UserInterfaceEditor::Reparent(const std::shared_ptr<UIElement>& node, const std::shared_ptr<UIElement>& newParent, int insertIndex) {
        if (!node || !newParent) return false;
        auto oldParent = ParentOf(node);
        if (oldParent.get() == newParent.get()) {
            return ReorderWithinParent(node, insertIndex);
        }
        if (oldParent) {
            auto& v = oldParent->Children();
            v.erase(std::remove_if(v.begin(), v.end(), [&](const std::shared_ptr<UIElement>& p) { return p.get() == node.get(); }), v.end());
        }
        auto& dest = newParent->Children();
        if (insertIndex < 0 || insertIndex >(int)dest.size()) dest.push_back(node);
        else dest.insert(dest.begin() + insertIndex, node);
        PushUndo("Reparent");
        return true;
    }

    bool UserInterfaceEditor::ReorderWithinParent(const std::shared_ptr<UIElement>& node, int newIndex) {
        auto parent = ParentOf(node);
        if (!parent) return false;
        auto& v = parent->Children();
        int cur = -1;
        for (int i = 0; i < (int)v.size(); ++i) if (v[i].get() == node.get()) { cur = i; break; }
        if (cur < 0) return false;
        if (newIndex < 0) newIndex = (int)v.size() - 1;
        newIndex = std::clamp(newIndex, 0, (int)v.size() - 1);
        if (cur == newIndex) return false;
        auto moved = v[cur];
        v.erase(v.begin() + cur);
        v.insert(v.begin() + newIndex, moved);
        PushUndo("Reorder");
        return true;
    }

    std::shared_ptr<UIElement> UserInterfaceEditor::CreateElement(UISerType type, const glm::vec2& pos) {
        std::shared_ptr<UIElement> e;
        switch (type) {
        case UISerType::Container:   e = std::make_shared<UIContainer>(MakeUniqueId("container", m_Root)); break;
        case UISerType::Button:      e = std::make_shared<UIButton>(MakeUniqueId("button", m_Root)); break;
        case UISerType::Text:        e = std::make_shared<UIText>(MakeUniqueId("text", m_Root)); break;
        case UISerType::Checkbox:    e = std::make_shared<UICheckbox>(MakeUniqueId("checkbox", m_Root)); break;
        case UISerType::ProgressBar: e = std::make_shared<UIProgressBar>(MakeUniqueId("progress", m_Root)); break;
        case UISerType::Slider:      e = std::make_shared<UISlider>(MakeUniqueId("slider", m_Root)); break;
        case UISerType::InputText:   e = std::make_shared<UITextInput>(MakeUniqueId("input", m_Root)); break;
        case UISerType::Image:       e = std::make_shared<UIImage>(MakeUniqueId("image", m_Root)); break;
        case UISerType::Separator:   e = std::make_shared<UISeparator>(MakeUniqueId("separator", m_Root)); break;
        case UISerType::TabBar:      e = std::make_shared<UITabBar>(MakeUniqueId("tabbar", m_Root)); break;
        case UISerType::Tabs:        e = std::make_shared<UITabs>(MakeUniqueId("tabs", m_Root)); break;
        case UISerType::Menu:        e = std::make_shared<UIMenu>(MakeUniqueId("menu", m_Root)); break;
        case UISerType::TooltipLayer:e = std::make_shared<UITooltipLayer>(MakeUniqueId("tooltips", m_Root)); break;
        default:                     e = std::make_shared<UIElement>(MakeUniqueId("element", m_Root)); break;
        }
        if (m_SnapToGrid) {
            float x = Snap(pos.x, m_GridStep);
            float y = Snap(pos.y, m_GridStep);
            e->Transform().pos = { x, y };
        }
        else {
            e->Transform().pos = pos;
        }
        e->Transform().size = { 160, 32 };
        if (auto t = dynamic_cast<UIText*>(e.get())) {
            t->text = "Text";
            e->Transform().size = { 140, 28 };
        }
        else if (dynamic_cast<UIButton*>(e.get())) {
            static_cast<UIButton*>(e.get())->label = "Button";
            e->Transform().size = { 120, 36 };
        }
        else if (auto cb = dynamic_cast<UICheckbox*>(e.get())) {
            cb->label = "Checkbox";
            e->Transform().size = { 160, 28 };
        }
        else if (auto pb = dynamic_cast<UIProgressBar*>(e.get())) {
            pb->min = 0; pb->max = 1; pb->value = 0.5f; pb->showText = false;
            e->Transform().size = { 220, 20 };
        }
        else if (auto img = dynamic_cast<UIImage*>(e.get())) {
            img->textureId.clear(); img->keepAspect = true; img->tint = { 1,1,1,1 };
            e->Transform().size = { 192, 128 };
        }
        else if (auto sl = dynamic_cast<UISlider*>(e.get())) {
            sl->min = 0; sl->max = 1; sl->value = 0.5f;
            e->Transform().size = { 220, 26 };
        }
        else if (auto in = dynamic_cast<UITextInput*>(e.get())) {
            in->text = ""; in->readOnly = false; in->minWidth = 80.f;
            e->Transform().size = { 240, 28 };
        }
        else if (dynamic_cast<UISeparator*>(e.get())) {
            e->Transform().size = { 240, 6 };
        }
        m_Root->AddChild(e);
        Select(e);
        PushUndo("Add element");
        return e;
    }

    void UserInterfaceEditor::Select(const std::shared_ptr<UIElement>& e) {
        m_Selected = e;
        m_SelectedIdCache = e ? e->Id() : std::string();
    }

    void UserInterfaceEditor::DeleteSelected() {
        auto sel = m_Selected.lock();
        if (!sel || sel.get() == m_Root.get()) return;
        auto parent = ParentOf(sel);
        if (!parent) return;
        auto& v = parent->Children();
        v.erase(std::remove_if(v.begin(), v.end(), [&](const std::shared_ptr<UIElement>& p) { return p.get() == sel.get(); }), v.end());
        m_Selected.reset();
        m_SelectedIdCache.clear();
        PushUndo("Delete");
    }

    void UserInterfaceEditor::DuplicateSelected() {
        auto sel = m_Selected.lock();
        if (!sel) return;
        std::vector<uint8_t> buf;
        if (!SerializeToBuffer(sel, buf)) return;
        auto dup = DeserializeFromBuffer(buf.data(), buf.size());
        if (!dup) return;
        dup->SetId(MakeUniqueId(sel->Id(), m_Root));
        dup->Transform().pos += glm::vec2(12.f, 12.f);
        auto parent = ParentOf(sel);
        if (!parent) parent = m_Root;
        parent->AddChild(dup);
        Select(dup);
        PushUndo("Duplicate");
    }

    void UserInterfaceEditor::BringToFront() {
        auto sel = m_Selected.lock();
        if (!sel) return;
        auto parent = ParentOf(sel);
        if (!parent) return;
        auto& v = parent->Children();
        auto it = std::find_if(v.begin(), v.end(), [&](const std::shared_ptr<UIElement>& p) { return p.get() == sel.get(); });
        if (it == v.end()) return;
        auto moved = *it;
        v.erase(it);
        v.push_back(moved);
        PushUndo("Z-Front");
    }

    void UserInterfaceEditor::SendToBack() {
        auto sel = m_Selected.lock();
        if (!sel) return;
        auto parent = ParentOf(sel);
        if (!parent) return;
        auto& v = parent->Children();
        auto it = std::find_if(v.begin(), v.end(), [&](const std::shared_ptr<UIElement>& p) { return p.get() == sel.get(); });
        if (it == v.end()) return;
        auto moved = *it;
        v.erase(it);
        v.insert(v.begin(), moved);
        PushUndo("Z-Back");
    }

    bool UserInterfaceEditor::HitTest(UIElement* e, ImVec2 p) const {
        const Rect& r = e->Transform().rect;
        ImRect R(ImVec2(r.x, r.y), ImVec2(r.x + r.w, r.y + r.h));
        return R.Contains(p);
    }

    int UserInterfaceEditor::HitTestHandle(UIElement* e, ImVec2 screen) const {
        const Rect& rr = e->Transform().rect;
        ImVec2 p0 = CanvasToScreen(ImVec2(rr.x, rr.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(rr.x + rr.w, rr.y + rr.h));
        ImVec2 c = (p0 + p1) * 0.5f;

        ImVec2 pts[8] = {
            p0, ImVec2(c.x,p0.y), ImVec2(p1.x,p0.y),
            ImVec2(p1.x,c.y),
            p1, ImVec2(c.x,p1.y), ImVec2(p0.x,p1.y),
            ImVec2(p0.x,c.y)
        };
        for (int i = 0; i < 8; ++i) {
            ImRect r(pts[i] - ImVec2(m_HandleSize, m_HandleSize), pts[i] + ImVec2(m_HandleSize, m_HandleSize));
            if (r.Contains(screen)) return i;
        }
        return -1;
    }

    void UserInterfaceEditor::StartDrag(ImVec2 mouseCanvas, UIElement* e) {
        m_Dragging = true;
        m_DragOffset = ImVec2(mouseCanvas.x - e->Transform().rect.x, mouseCanvas.y - e->Transform().rect.y);
    }

    void UserInterfaceEditor::StartResize(int handleIndex) {
        m_Resizing = true;
        m_ResizeHandle = handleIndex;
    }

    void UserInterfaceEditor::ApplyDrag(ImVec2 mouseCanvas) {
        auto sel = m_Selected.lock();
        if (!sel) return;
        Rect& r = sel->Transform().rect;
        float nx = mouseCanvas.x - m_DragOffset.x;
        float ny = mouseCanvas.y - m_DragOffset.y;
        if (m_SnapToGrid) { nx = Snap(nx, m_GridStep); ny = Snap(ny, m_GridStep); }
        r.x = nx; r.y = ny;
        ClampRect(r);
    }

    void UserInterfaceEditor::ApplyResize(ImVec2 mouseCanvas) {
        auto sel = m_Selected.lock();
        if (!sel || m_ResizeHandle < 0) return;
        auto& r = sel->Transform().rect;
        float minW = 12.f, minH = 8.f;

        switch (m_ResizeHandle) {
        case 0: {
            float rx = mouseCanvas.x; float ry = mouseCanvas.y;
            if (m_SnapToGrid) { rx = Snap(rx, m_GridStep); ry = Snap(ry, m_GridStep); }
            float dx = r.x + r.w - rx; float dy = r.y + r.h - ry;
            r.x = rx; r.y = ry; r.w = std::max(minW, dx); r.h = std::max(minH, dy);
        } break;
        case 1: {
            float ry = mouseCanvas.y; if (m_SnapToGrid) ry = Snap(ry, m_GridStep);
            float dy = r.y + r.h - ry; r.y = ry; r.h = std::max(minH, dy);
        } break;
        case 2: {
            float ry = mouseCanvas.y; if (m_SnapToGrid) ry = Snap(ry, m_GridStep);
            float dx = mouseCanvas.x - r.x; r.w = std::max(minW, m_SnapToGrid ? Snap(dx, m_GridStep) : dx);
            float dy = r.y + r.h - ry; r.y = ry; r.h = std::max(minH, dy);
        } break;
        case 3: { float dx = mouseCanvas.x - r.x; r.w = std::max(minW, m_SnapToGrid ? Snap(dx, m_GridStep) : dx); } break;
        case 4: {
            float dx = mouseCanvas.x - r.x; float dy = mouseCanvas.y - r.y;
            r.w = std::max(minW, m_SnapToGrid ? Snap(dx, m_GridStep) : dx);
            r.h = std::max(minH, m_SnapToGrid ? Snap(dy, m_GridStep) : dy);
        } break;
        case 5: { float dy = mouseCanvas.y - r.y; r.h = std::max(minH, m_SnapToGrid ? Snap(dy, m_GridStep) : dy); } break;
        case 6: {
            float rx = mouseCanvas.x; if (m_SnapToGrid) rx = Snap(rx, m_GridStep);
            float dy = mouseCanvas.y - r.y; r.h = std::max(minH, m_SnapToGrid ? Snap(dy, m_GridStep) : dy);
            float dx = r.x + r.w - rx; r.x = rx; r.w = std::max(minW, dx);
        } break;
        case 7: {
            float rx = mouseCanvas.x; if (m_SnapToGrid) rx = Snap(rx, m_GridStep);
            float dx = r.x + r.w - rx; r.x = rx; r.w = std::max(minW, dx);
        } break;
        default: break;
        }
        ClampRect(r);
    }

    void UserInterfaceEditor::DrawHierarchyPanel(float, float) {
        auto selected = m_Selected.lock();

        if (ImGui::Button("+ Button")) { CreateElement(UISerType::Button, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Text")) { CreateElement(UISerType::Text, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Checkbox")) { CreateElement(UISerType::Checkbox, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Progress")) { CreateElement(UISerType::ProgressBar, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Image")) { CreateElement(UISerType::Image, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Slider")) { CreateElement(UISerType::Slider, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }
        ImGui::SameLine();
        if (ImGui::Button("+ Input")) { CreateElement(UISerType::InputText, ToGlm(ScreenToCanvas(ImGui::GetMousePos()))); }

        ImGui::Separator();
        if (!m_Root) {
            ImGui::TextDisabled("Pas de racine UI.");
            return;
        }

        ImGui::Text("Elements");
        ImGui::BeginChild("##list", ImVec2(-1, -1), false);

        std::function<void(std::shared_ptr<UIElement>, int)> drawNode = [&](std::shared_ptr<UIElement> n, int depth) {
            bool sel = (selected && n.get() == selected.get());
            ImGui::Indent(depth * 10.f);
            std::string label = std::string(SerTypeToString(UITypeOf(n.get()))) + "  " + n->Id();
            bool clicked = TreeNodeExFlat(label.c_str(), sel);
            if (clicked) Select(n);

            if (ImGui::BeginDragDropSource()) {
                UIElement* raw = n.get();
                ImGui::SetDragDropPayload("QUI_NODE", &raw, sizeof(UIElement*));
                ImGui::Text("Deplacer: %s", n->Id().c_str());
                ImGui::EndDragDropSource();
            }
            
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("QUI_NODE")) {
                    UIElement* raw = *(UIElement**)p->Data;
                    auto src = FindById(raw->Id());
                    if (src && src.get() != n.get()) {
                        if (ImGui::GetIO().KeyAlt) {
                            auto parent = ParentOf(n);
                            if (parent) {
                                auto& kids = parent->Children();
                                int idx = 0; for (; idx < (int)kids.size(); ++idx) if (kids[idx].get() == n.get()) break;
                                Reparent(src, parent, idx);
                            }
                        }
                        else {
                            Reparent(src, n, -1);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            for (auto& c : n->Children()) drawNode(c, depth + 1);
            ImGui::Unindent(depth * 10.f);
            };

        drawNode(m_Root, 0);
        ImGui::EndChild();
    }

    void UserInterfaceEditor::DrawCanvasPanel(float, float) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        m_CanvasPos = ImGui::GetCursorScreenPos();
        m_CanvasSize = ImGui::GetContentRegionAvail();

        dl->AddRectFilled(m_CanvasPos, m_CanvasPos + m_CanvasSize, IM_COL32(22, 24, 28, 255), 6.0f);
        dl->AddRect(m_CanvasPos, m_CanvasPos + m_CanvasSize, IM_COL32(90, 90, 100, 180), 6.0f);

        ImVec2 v0 = CanvasToScreen(ImVec2(0, 0));
        ImVec2 v1 = CanvasToScreen(ImVec2((float)m_DesignW, (float)m_DesignH));

        ImU32 dim = IM_COL32(10, 10, 12, 140);
        dl->AddRectFilled(m_CanvasPos, ImVec2(v0.x, m_CanvasPos.y + m_CanvasSize.y), dim);
        dl->AddRectFilled(ImVec2(v1.x, m_CanvasPos.y), m_CanvasPos + m_CanvasSize, dim);
        dl->AddRectFilled(ImVec2(v0.x, m_CanvasPos.y), ImVec2(v1.x, v0.y), dim);
        dl->AddRectFilled(ImVec2(v0.x, v1.y), ImVec2(v1.x, m_CanvasPos.y + m_CanvasSize.y), dim);

        ImU32 border = IM_COL32(255, 210, 80, 255);
        dl->AddRect(v0, v1, border, 0.f, 0, 2.0f);
        const float corner = 14.0f * m_Zoom;
        dl->AddLine(v0, ImVec2(v0.x + corner, v0.y), border, 2.0f);
        dl->AddLine(v0, ImVec2(v0.x, v0.y + corner), border, 2.0f);
        dl->AddLine(ImVec2(v1.x, v0.y), ImVec2(v1.x - corner, v0.y), border, 2.0f);
        dl->AddLine(ImVec2(v1.x, v0.y), ImVec2(v1.x, v0.y + corner), border, 2.0f);
        dl->AddLine(ImVec2(v0.x, v1.y), ImVec2(v0.x + corner, v1.y), border, 2.0f);
        dl->AddLine(ImVec2(v0.x, v1.y), ImVec2(v0.x, v1.y - corner), border, 2.0f);
        dl->AddLine(v1, ImVec2(v1.x - corner, v1.y), border, 2.0f);
        dl->AddLine(v1, ImVec2(v1.x, v1.y - corner), border, 2.0f);

        char tag[64];
        std::snprintf(tag, sizeof(tag), "%ux%u", m_DesignW, m_DesignH);
        ImVec2 ts = ImGui::CalcTextSize(tag);
        dl->AddRectFilled(ImVec2(v0.x + 8, v0.y + 8), ImVec2(v0.x + 12 + ts.x, v0.y + 12 + ts.y), IM_COL32(22, 24, 28, 220), 4.f);
        dl->AddText(ImVec2(v0.x + 10, v0.y + 10), IM_COL32(255, 230, 160, 255), tag);

        ImGui::InvisibleButton("##CanvasIO", m_CanvasSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hovering = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        if (hovering) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Middle] = ImGui::GetIO().MousePos;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                m_Pan += ImGui::GetIO().MouseDelta;

            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                float prev = m_Zoom;
                m_Zoom = std::clamp(m_Zoom + wheel * 0.1f, 0.4f, 4.0f);
                ImVec2 mouse = ImGui::GetIO().MousePos;
                m_Pan = (m_Pan - mouse) * (m_Zoom / prev) + mouse;
            }
        }

        if (m_ShowGrid) DrawGrid(dl, m_CanvasPos, m_CanvasSize, m_GridStep * m_Zoom);

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("QUI_CanvasMenu");
        if (ImGui::BeginPopup("QUI_CanvasMenu")) {
            ImVec2 mc = ScreenToCanvas(ImGui::GetIO().MousePos);
            auto add = [&](const char* name, UISerType t) { if (ImGui::MenuItem(name)) CreateElement(t, ToGlm(mc)); };
            add("Ajouter Button", UISerType::Button);
            add("Ajouter Text", UISerType::Text);
            add("Ajouter Checkbox", UISerType::Checkbox);
            add("Ajouter ProgressBar", UISerType::ProgressBar);
            add("Ajouter Separator", UISerType::Separator);
            add("Ajouter Image", UISerType::Image);
            add("Ajouter Slider", UISerType::Slider);
            add("Ajouter InputText", UISerType::InputText);
            add("Ajouter Container", UISerType::Container);
            add("Ajouter TabBar", UISerType::TabBar);
            add("Ajouter Tabs", UISerType::Tabs);
            add("Ajouter Menu", UISerType::Menu);
            add("Ajouter TooltipLayer", UISerType::TooltipLayer);
            ImGui::Separator();
            if (ImGui::MenuItem("Dupliquer", nullptr, false, !m_Selected.expired())) DuplicateSelected();
            if (ImGui::MenuItem("Supprimer", nullptr, false, !m_Selected.expired())) DeleteSelected();
            ImGui::EndPopup();
        }

        std::vector<UIElement*> draw;
        FlattenZOrder(m_Root, draw);
        for (auto* e : draw) {
            bool sel = false;
            if (auto s = m_Selected.lock()) sel = (e == s.get());
            DrawElementBox(dl, e, sel);
            if (sel) DrawResizeHandles(dl, e);
        }

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mc = ScreenToCanvas(ImGui::GetIO().MousePos);
            int handle = -1;
            UIElement* hit = nullptr;
            for (int i = (int)draw.size() - 1; i >= 0; --i) {
                handle = HitTestHandle(draw[i], ImGui::GetIO().MousePos);
                if (handle >= 0) { hit = draw[i]; break; }
                if (HitTest(draw[i], mc)) { hit = draw[i]; break; }
            }
            if (hit) {
                auto ptr = FindById(hit->Id());
                Select(ptr);
                if (handle >= 0) StartResize(handle);
                else StartDrag(mc, hit);
            }
            else {
                Select({});
            }
        }

        if (active) {
            if (m_Dragging)  ApplyDrag(ScreenToCanvas(ImGui::GetIO().MousePos));
            if (m_Resizing)  ApplyResize(ScreenToCanvas(ImGui::GetIO().MousePos));
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (m_Dragging || m_Resizing) PushUndo("Transform");
            m_Dragging = false;
            m_Resizing = false;
            m_ResizeHandle = -1;
        }
    }

    void UserInterfaceEditor::DrawGrid(ImDrawList* dl, ImVec2 origin, ImVec2 size, float step) const {
        ImU32 minor = IM_COL32(60, 62, 68, 120);
        for (float x = fmodf(m_Pan.x, step); x < size.x; x += step)
            dl->AddLine(origin + ImVec2(x, 0), origin + ImVec2(x, size.y), minor);
        for (float y = fmodf(m_Pan.y, step); y < size.y; y += step)
            dl->AddLine(origin + ImVec2(0, y), origin + ImVec2(size.x, y), minor);
    }

    void UserInterfaceEditor::DrawElementBox(ImDrawList* dl, UIElement* e, bool selected) const {
        const Rect& r = e->Transform().rect;
        ImVec2 p0 = CanvasToScreen(ImVec2(r.x, r.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(r.x + r.w, r.y + r.h));

        const UIColor bg = e->Style().bg;
        const UIColor fg = e->Style().fg;

        ImU32 fill = selected ? IM_COL32(54, 132, 255, 40) : ToImColor(UIColor{ bg.r,bg.g,bg.b, bg.a > 0 ? bg.a : 0.12f });
        ImU32 border = selected ? IM_COL32(70, 160, 255, 255) : IM_COL32(160, 160, 170, 180);

        dl->AddRectFilled(p0, p1, fill, e->Style().radius);
        dl->AddRect(p0, p1, border, e->Style().radius, 0, 2.0f);

        std::string title = (std::string(SerTypeToString(UITypeOf(e))) + "  " + e->Id());
        ImVec2 ts = ImGui::CalcTextSize(title.c_str());
        ImVec2 tc = (p0 + p1) * 0.5f - ts * 0.5f;
        dl->AddText(tc, ToImColor(fg.a > 0 ? fg : UIColor{ 0.9f,0.9f,0.95f,1.f }), title.c_str());
    }

    void UserInterfaceEditor::DrawResizeHandles(ImDrawList* dl, UIElement* e) const {
        const Rect& r = e->Transform().rect;
        ImVec2 p0 = CanvasToScreen(ImVec2(r.x, r.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(r.x + r.w, r.y + r.h));
        ImVec2 c = (p0 + p1) * 0.5f;

        ImVec2 pts[8] = {
            p0, ImVec2(c.x,p0.y), ImVec2(p1.x,p0.y),
            ImVec2(p1.x,c.y),
            p1, ImVec2(c.x,p1.y), ImVec2(p0.x,p1.y),
            ImVec2(p0.x,c.y)
        };
        for (auto& pt : pts)
            dl->AddRectFilled(pt - ImVec2(m_HandleSize, m_HandleSize), pt + ImVec2(m_HandleSize, m_HandleSize), IM_COL32(250, 220, 60, 220), 2.0f);
    }

    void UserInterfaceEditor::DrawCommonProperties(UIElement& e, bool& anyChanged) {
        ImGui::SeparatorText("Identite");
        ImGui::TextDisabled("Type: %s", SerTypeToString(UITypeOf(&e)));
        ImGui::Text("Id: %s", e.Id().c_str());

        ImGui::SeparatorText("Transform");
        Rect rc = e.Transform().rect;
        float x = rc.x, y = rc.y, w = rc.w, h = rc.h;
        if (ImGui::DragFloat("X", &x, 1.f)) { rc.x = m_SnapToGrid ? Snap(x, m_GridStep) : x; anyChanged = true; }
        if (ImGui::DragFloat("Y", &y, 1.f)) { rc.y = m_SnapToGrid ? Snap(y, m_GridStep) : y; anyChanged = true; }
        if (ImGui::DragFloat("W", &w, 1.f, 2.f, 8192.f)) { rc.w = std::max(2.f, m_SnapToGrid ? Snap(w, m_GridStep) : w); anyChanged = true; }
        if (ImGui::DragFloat("H", &h, 1.f, 2.f, 8192.f)) { rc.h = std::max(2.f, m_SnapToGrid ? Snap(h, m_GridStep) : h); anyChanged = true; }
        if (anyChanged) {
            e.Transform().rect = rc;
            ClampRect(e.Transform().rect);
        }

        ImGui::SeparatorText("Style");
        auto& st = e.Style();
        anyChanged |= ImGui::DragFloat("Padding", &st.padding, 0.25f, 0.f, 64.f);
        anyChanged |= ImGui::DragFloat("Radius", &st.radius, 0.25f, 0.f, 64.f);
        float bg[4] = { st.bg.r, st.bg.g, st.bg.b, st.bg.a };
        float fg[4] = { st.fg.r, st.fg.g, st.fg.b, st.fg.a };
        if (ImGui::ColorEdit4("BG", bg)) { st.bg = { bg[0], bg[1], bg[2], bg[3] }; anyChanged = true; }
        if (ImGui::ColorEdit4("FG", fg)) { st.fg = { fg[0], fg[1], fg[2], fg[3] }; anyChanged = true; }
    }

    void UserInterfaceEditor::DrawTypedProperties(UIElement& e, bool& anyChanged) {
        if (auto* b = dynamic_cast<UIButton*>(&e)) {
            char buf[256]; std::strncpy(buf, b->label.c_str(), sizeof(buf));
            if (ImGui::InputText("Label", buf, IM_ARRAYSIZE(buf))) { b->label = buf; anyChanged = true; }
        }
        if (auto* t = dynamic_cast<UIText*>(&e)) {
            char buf[512]; std::strncpy(buf, t->text.c_str(), sizeof(buf));
            if (ImGui::InputTextMultiline("Text", buf, IM_ARRAYSIZE(buf))) { t->text = buf; anyChanged = true; }
            anyChanged |= ImGui::DragFloat("Font size", &t->fontSize, 0.25f, 6.f, 128.f);
        }
        if (auto* c = dynamic_cast<UICheckbox*>(&e)) {
            char buf[256]; std::strncpy(buf, c->label.c_str(), sizeof(buf));
            if (ImGui::InputText("Label", buf, IM_ARRAYSIZE(buf))) { c->label = buf; anyChanged = true; }
            bool chk = c->checked;
            if (ImGui::Checkbox("Checked (default)", &chk)) { c->checked = chk; anyChanged = true; }
        }
        if (auto* p = dynamic_cast<UIProgressBar*>(&e)) {
            anyChanged |= ImGui::DragFloat("Min", &p->min, 0.1f);
            anyChanged |= ImGui::DragFloat("Max", &p->max, 0.1f);
            p->max = std::max(p->max, p->min);
            anyChanged |= ImGui::SliderFloat("Value", &p->value, p->min, p->max);
            bool show = p->showText;
            if (ImGui::Checkbox("Show text", &show)) { p->showText = show; anyChanged = true; }
        }
        if (auto* s = dynamic_cast<UISlider*>(&e)) {
            anyChanged |= ImGui::DragFloat("Min", &s->min, 0.1f);
            anyChanged |= ImGui::DragFloat("Max", &s->max, 0.1f);
            s->max = std::max(s->max, s->min);
            anyChanged |= ImGui::SliderFloat("Value", &s->value, s->min, s->max);
        }
        if (auto* it = dynamic_cast<UITextInput*>(&e)) {
            char buf[512]; std::strncpy(buf, it->text.c_str(), sizeof(buf));
            if (ImGui::InputText("Text (preview)", buf, IM_ARRAYSIZE(buf))) { it->text = buf; anyChanged = true; }
            bool ro = it->readOnly; if (ImGui::Checkbox("Read only", &ro)) { it->readOnly = ro; anyChanged = true; }
            anyChanged |= ImGui::DragFloat("Min width", &it->minWidth, 1.f, 0.f, 2048.f);
        }
        if (auto* im = dynamic_cast<UIImage*>(&e)) {
            char tid[260]; std::strncpy(tid, im->textureId.c_str(), sizeof(tid));
            if (ImGui::InputText("TextureId", tid, IM_ARRAYSIZE(tid))) { im->textureId = tid; anyChanged = true; }
            bool ka = im->keepAspect; if (ImGui::Checkbox("Keep aspect", &ka)) { im->keepAspect = ka; anyChanged = true; }
            float tint[4] = { im->tint.r, im->tint.g, im->tint.b, im->tint.a };
            if (ImGui::ColorEdit4("Tint", tint, ImGuiColorEditFlags_NoInputs)) {
                im->tint = { tint[0], tint[1], tint[2], tint[3] }; anyChanged = true;
            }
        }
        if (auto* sp = dynamic_cast<UISeparator*>(&e)) {
            anyChanged |= ImGui::DragFloat("Thickness", &sp->thickness, 0.1f, 0.5f, 16.f);
            float col[4] = { sp->color.r, sp->color.g, sp->color.b, sp->color.a };
            if (ImGui::ColorEdit4("Color", col)) { sp->color = { col[0],col[1],col[2],col[3] }; anyChanged = true; }
        }
        if (auto* ct = dynamic_cast<UIContainer*>(&e)) {
            int layout = (ct->layout == UILayoutType::Horizontal) ? 1 : 0;
            if (ImGui::Combo("Layout", &layout, "Vertical\0Horizontal\0\0")) {
                ct->layout = (layout == 1) ? UILayoutType::Horizontal : UILayoutType::Vertical;
                anyChanged = true;
            }
            anyChanged |= ImGui::DragFloat("Gap", &ct->gap, 0.25f, 0.f, 128.f);
        }
        if (auto* tb = dynamic_cast<UITabBar*>(&e)) {
            anyChanged |= ImGui::DragInt("Selected", &tb->selected, 1, 0, 1024);
            if (ImGui::Button("+ Tab")) { tb->labels.push_back("Tab"); anyChanged = true; }
            for (size_t i = 0; i < tb->labels.size(); ++i) {
                ImGui::PushID((int)i);
                char buf[128]; std::strncpy(buf, tb->labels[i].c_str(), sizeof(buf));
                if (ImGui::InputText("Label", buf, IM_ARRAYSIZE(buf))) { tb->labels[i] = buf; anyChanged = true; }
                ImGui::SameLine();
                if (ImGui::Button("X")) { tb->labels.erase(tb->labels.begin() + i); anyChanged = true; ImGui::PopID(); break; }
                ImGui::PopID();
            }
        }
        if (auto* ts = dynamic_cast<UITabs*>(&e)) {
            anyChanged |= ImGui::DragInt("Selected", &ts->m_Selected, 1, 0, 1024);
        }
        if (auto* mu = dynamic_cast<UIMenu*>(&e)) {
            if (ImGui::Button("+ Item")) { mu->items.push_back({ "Item", false }); anyChanged = true; }
            for (size_t i = 0; i < mu->items.size(); ++i) {
                ImGui::PushID((int)i);
                char buf[128]; std::strncpy(buf, mu->items[i].label.c_str(), sizeof(buf));
                if (ImGui::InputText("Label", buf, IM_ARRAYSIZE(buf))) { mu->items[i].label = buf; anyChanged = true; }
                bool dis = mu->items[i].disabled; if (ImGui::Checkbox("Disabled", &dis)) { mu->items[i].disabled = dis; anyChanged = true; }
                if (ImGui::Button("Supprimer")) { mu->items.erase(mu->items.begin() + i); anyChanged = true; ImGui::PopID(); break; }
                ImGui::PopID();
            }
        }
    }

    void UserInterfaceEditor::DrawPropertiesPanel(float, float) {
        auto sel = m_Selected.lock();
        if (!sel) {
            ImGui::TextDisabled("Aucun element selectionne.");
            return;
        }
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.45f, 1.f), "%s  (%s)", SerTypeToString(UITypeOf(sel.get())), sel->Id().c_str());
        ImGui::Separator();

        bool changed = false;
        DrawCommonProperties(*sel, changed);
        DrawTypedProperties(*sel, changed);

        if (changed) {
            PushUndo("Edit properties");
        }
    }

    void UserInterfaceEditor::DrawPreviewPanel(float, float) {
        ImGui::Text("Previsualisation");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        if (!m_DesignW || !m_DesignH) {
            ImGui::TextDisabled("Design resolution invalide.");
            return;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginChild("##preview_frame", ImVec2(-1, -1), true, flags)) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float rw = static_cast<float>(m_DesignW);
            float rh = static_cast<float>(m_DesignH);
            float scale = std::min(avail.x / rw, avail.y / rh);
            scale = std::max(0.1f, scale);

            ImVec2 previewSize = ImVec2(rw * scale, rh * scale);
            ImVec2 topLeft = ImVec2((avail.x - previewSize.x) * 0.5f, (avail.y - previewSize.y) * 0.5f);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 base = ImGui::GetCursorScreenPos();
            ImVec2 p0 = base + topLeft;
            ImVec2 p1 = p0 + previewSize;

            dl->AddRectFilled(p0, p1, IM_COL32(18, 20, 24, 255), 6.f);
            ImGui::PushClipRect(p0, p1, true);
            RenderRuntimePreview(scale, topLeft);
            ImGui::PopClipRect();
        }
        ImGui::EndChild();
    }

    void UserInterfaceEditor::RenderRuntimePreview(float scale, ImVec2 offset) const {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 base = ImGui::GetCursorScreenPos() + offset;

        std::vector<UIElement*> draw;
        FlattenZOrder(m_Root, draw);

        for (auto* e : draw) {
            const Rect& R = e->Transform().rect;
            ImVec2 rmin = base + ImVec2(R.x * scale, R.y * scale);
            ImVec2 rsz = ImVec2(R.w * scale, R.h * scale);
            ImVec2 rmax = rmin + rsz;

            const auto& st = e->Style();
            ImU32 bg = ToImColor(st.bg.a > 0 ? st.bg : UIColor{ 0.14f,0.16f,0.2f,1.f });
            ImU32 fg = ToImColor(st.fg.a > 0 ? st.fg : UIColor{ 0.9f,0.9f,0.95f,1.f });

            auto type = UITypeOf(e);
            switch (type) {
            case UISerType::Text: {
                auto* t = static_cast<UIText*>(e);
                float currentPx = ImGui::GetFontSize();
                float desiredPx = (t->fontSize > 0.0f ? t->fontSize : currentPx);
                float fs = desiredPx / currentPx; if (fs < 0.01f) fs = 0.01f;
                FontScaleScope fss(fs);
                dl->AddText(rmin + ImVec2(4, 4), fg, t->text.c_str());
            } break;

            case UISerType::Button: {
                auto* b = static_cast<UIButton*>(e);
                dl->AddRectFilled(rmin, rmax, IM_COL32(50, 60, 75, 255), st.radius);
                dl->AddRect(rmin, rmax, IM_COL32(90, 110, 140, 255), st.radius, 0, 2.0f);
                ImVec2 ts = ImGui::CalcTextSize(b->label.c_str());
                ImVec2 tp = rmin + (rsz - ts) * 0.5f;
                dl->AddText(tp, fg, b->label.c_str());
            } break;

            case UISerType::Checkbox: {
                auto* c = static_cast<UICheckbox*>(e);
                float boxSide = ImClamp(rsz.y * 0.7f, 10.0f, 28.0f);
                ImVec2 c0 = rmin + ImVec2(6, (rsz.y - boxSide) * 0.5f);
                ImVec2 c1 = c0 + ImVec2(boxSide, boxSide);
                dl->AddRectFilled(c0, c1, IM_COL32(40, 48, 60, 255), 3.f);
                dl->AddRect(c0, c1, IM_COL32(90, 110, 140, 255), 3.f, 0, 1.5f);
                if (c->checked) {
                    ImVec2 a = ImVec2(c0.x + boxSide * 0.22f, c0.y + boxSide * 0.55f);
                    ImVec2 b2 = ImVec2(c0.x + boxSide * 0.42f, c0.y + boxSide * 0.75f);
                    ImVec2 c2 = ImVec2(c0.x + boxSide * 0.78f, c0.y + boxSide * 0.28f);
                    dl->AddLine(a, b2, IM_COL32(240, 240, 245, 255), 2.0f);
                    dl->AddLine(b2, c2, IM_COL32(240, 240, 245, 255), 2.0f);
                }
                dl->AddText(ImVec2(c1.x + 8, rmin.y + (rsz.y - ImGui::GetTextLineHeight()) * 0.5f), fg, c->label.c_str());
            } break;

            case UISerType::ProgressBar: {
                auto* p = static_cast<UIProgressBar*>(e);
                dl->AddRectFilled(rmin, rmax, IM_COL32(40, 46, 56, 255), 4.f);
                float t = 0.f;
                if (p->max > p->min) t = (p->value - p->min) / (p->max - p->min);
                t = Saturate(t);
                ImVec2 f1 = ImVec2(rmin.x + rsz.x * t, rmax.y);
                dl->AddRectFilled(rmin, f1, IM_COL32(90, 170, 240, 255), 4.f);
                if (p->showText) {
                    char tmp[64]; std::snprintf(tmp, sizeof(tmp), "%.0f %%", t * 100.f);
                    ImVec2 ts = ImGui::CalcTextSize(tmp);
                    ImVec2 tp = rmin + (rsz - ts) * 0.5f;
                    dl->AddText(tp, fg, tmp);
                }
            } break;

            case UISerType::Slider: {
                auto* s = static_cast<UISlider*>(e);
                dl->AddRectFilled(rmin, rmax, IM_COL32(45, 50, 60, 255), 4.f);
                float t = 0.f;
                if (s->max > s->min) t = (s->value - s->min) / (s->max - s->min);
                t = Saturate(t);
                float hx = rmin.x + rsz.x * t;
                float handleW = std::max(6.0f, rsz.y * 0.5f);
                ImVec2 h0 = ImVec2(hx - handleW * 0.5f, rmin.y);
                ImVec2 h1 = ImVec2(hx + handleW * 0.5f, rmax.y);
                dl->AddRectFilled(h0, h1, IM_COL32(230, 230, 240, 255), 3.f);
                dl->AddRect(h0, h1, IM_COL32(40, 45, 55, 200), 3.f, 0, 1.0f);
            } break;

            case UISerType::InputText: {
                auto* it = static_cast<UITextInput*>(e);
                dl->AddRectFilled(rmin, rmax, IM_COL32(35, 40, 50, 255), 4.f);
                dl->AddRect(rmin, rmax, IM_COL32(90, 110, 140, 255), 4.f, 0, 1.5f);
                std::string shown = it->text;
                ImVec2 tp = rmin + ImVec2(6, (rsz.y - ImGui::GetTextLineHeight()) * 0.5f);
                dl->AddText(tp, fg, shown.c_str());
            } break;

            case UISerType::Image: {
                auto* im = static_cast<UIImage*>(e);
                ImU32 col = IM_COL32((int)(im->tint.r * 80 + 20), (int)(im->tint.g * 80 + 20), (int)(im->tint.b * 80 + 20), (int)(im->tint.a * 255));
                dl->AddRectFilled(rmin, rmax, col, 4.f);
                dl->AddRect(rmin, rmax, IM_COL32(180, 180, 200, 200), 4.f, 0, 1.5f);
                const char* txt = im->textureId.empty() ? "Image" : im->textureId.c_str();
                ImVec2 ts = ImGui::CalcTextSize(txt);
                ImVec2 tp = rmin + (rsz - ts) * 0.5f;
                dl->AddText(tp, IM_COL32(240, 240, 245, 220), txt);
            } break;

            case UISerType::Separator: {
                auto* sp = static_cast<UISeparator*>(e);
                float y = rmin.y + rsz.y * 0.5f;
                dl->AddLine(ImVec2(rmin.x, y), ImVec2(rmax.x, y), ToImColor(sp->color), sp->thickness);
            } break;

            default: {
                dl->AddRectFilled(rmin, rmax, bg, st.radius);
                dl->AddRect(rmin, rmax, IM_COL32(160, 160, 170, 180), st.radius, 0, 1.5f);
            } break;
            }
        }
    }

    ImVec2 UserInterfaceEditor::ScreenToCanvas(ImVec2 screen) const {
        ImVec2 p = screen - m_CanvasPos;
        return (p - m_Pan) / m_Zoom;
    }
    ImVec2 UserInterfaceEditor::CanvasToScreen(ImVec2 canvas) const {
        ImVec2 p = canvas * m_Zoom + m_Pan + m_CanvasPos;
        return p;
    }
    void UserInterfaceEditor::ClampRect(Rect& r) const {
        r.w = std::max(2.f, r.w);
        r.h = std::max(2.f, r.h);
        r.x = std::max(-4096.f, std::min(4096.f, r.x));
        r.y = std::max(-4096.f, std::min(4096.f, r.y));
    }

    bool UserInterfaceEditor::SerializeToBuffer(const std::shared_ptr<UIElement>& root, std::vector<uint8_t>& out) const {
        /*const char* tmp = "__ui_undo_redo_buffer.qui";
        UISaveOptions opt{};
        if (!UISaveToFile(root, tmp, opt)) return false;
        std::ifstream f(tmp, std::ios::binary);
        if (!f) return false;
        f.seekg(0, std::ios::end);
        std::streamsize sz = f.tellg();
        f.seekg(0, std::ios::beg);
        out.resize((size_t)sz);
        if (sz > 0) f.read((char*)out.data(), sz);*/
        return true;
    }

    std::shared_ptr<UIElement> UserInterfaceEditor::DeserializeFromBuffer(const uint8_t* data, size_t size) const {
        /*const char* tmp = "__ui_undo_redo_buffer.qui";
        std::ofstream fo(tmp, std::ios::binary);
        if (!fo) return {};
        fo.write((const char*)data, (std::streamsize)size);
        fo.close();
        return UILoadFromFile(tmp);*/
		return {};
    }

    void UserInterfaceEditor::PushUndo(const char*) {
        std::vector<uint8_t> buf;
        if (!SerializeToBuffer(m_Root, buf)) return;
        m_Undo.push_back(std::move(buf));
        m_Redo.clear();
    }

    void UserInterfaceEditor::DoUndo() {
        if (m_Undo.size() <= 1) return;
        auto cur = std::move(m_Undo.back()); m_Undo.pop_back();
        m_Redo.push_back(std::move(cur));
        auto& prev = m_Undo.back();
        auto root = DeserializeFromBuffer(prev.data(), prev.size());
        if (!root) return;
        m_Root = std::move(root);
        if (!m_SelectedIdCache.empty()) Select(FindById(m_SelectedIdCache));
    }

    void UserInterfaceEditor::DoRedo() {
        if (m_Redo.empty()) return;
        auto next = std::move(m_Redo.back()); m_Redo.pop_back();
        m_Undo.push_back(next);
        auto& last = m_Undo.back();
        auto root = DeserializeFromBuffer(last.data(), last.size());
        if (!root) return;
        m_Root = std::move(root);
        if (!m_SelectedIdCache.empty()) Select(FindById(m_SelectedIdCache));
    }
}
