#include "UserInterfaceEditor.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    UserInterfaceEditor::UserInterfaceEditor()
    {
        New();
    }

    bool UserInterfaceEditor::New()
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        m_Elements.clear();
        m_SelectedIndex = -1;
        m_NextId = 1;
        m_Undo.clear();
        m_Redo.clear();
        m_OpenedPath.clear();
        
        UIElement t;
        t.id = m_NextId++;
        t.type = UIType::Text;
        t.rect = { 24, 24, 220, 28 };
        std::snprintf(t.label, sizeof(t.label), "Nouvelle interface");
        m_Elements.push_back(t);
        PushUndo("New");
        return true;
    }

    bool UserInterfaceEditor::LoadFromFile(const char* path)
    {
        if (!path) return false;
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (!Deserialize(path)) return false;
        m_OpenedPath = path;
        m_Undo.clear(); m_Redo.clear();
        PushUndo("Load");
        return true;
    }

    bool UserInterfaceEditor::SaveToFile(const char* path) const
    {
        if (!path) return false;
        std::lock_guard<std::mutex> lk(m_Mutex);
        bool ok = Serialize(path);
        return ok;
    }

    void UserInterfaceEditor::SetDesignResolution(uint32_t w, uint32_t h)
    {
        m_DesignW = (w == 0 ? 1u : w);
        m_DesignH = (h == 0 ? 1u : h);
    }

    void UserInterfaceEditor::OnImGuiRender(const char* windowName)
    {
        ImGui::SetNextWindowSize(ImVec2(1200, 720), ImGuiCond_Once);
        ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        DrawMainToolbar();
        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float leftRatio = 0.22f;
        static float rightRatio = 0.28f;
        float minLeft = 240.f, minRight = 280.f;
        float leftW = ImMax(avail.x * leftRatio, minLeft);
        float rightW = ImMax(avail.x * rightRatio, minRight);
        float centerW = ImMax(avail.x - leftW - rightW - 12.f, 320.f);

        ImGui::BeginChild("##Hierarchy", ImVec2(leftW, 0), true);
        DrawHierarchyPanel(leftW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##split1", ImVec2(6, avail.y));
        if (ImGui::IsItemActive())
        {
            leftRatio += ImGui::GetIO().MouseDelta.x / avail.x;
            leftRatio = std::clamp(leftRatio, 0.12f, 0.5f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Canvas", ImVec2(centerW, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        DrawCanvasPanel(centerW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##split2", ImVec2(6, avail.y));
        if (ImGui::IsItemActive())
        {
            rightRatio -= ImGui::GetIO().MouseDelta.x / avail.x;
            rightRatio = std::clamp(rightRatio, 0.18f, 0.6f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Right", ImVec2(rightW, 0), true);
        {
            ImVec2 rAvail = ImGui::GetContentRegionAvail();
            static float propRatio = 0.58f;
            float propH = ImMax(rAvail.y * propRatio, 180.f);

            ImGui::BeginChild("##Props", ImVec2(-1, propH), true);
            DrawPropertiesPanel(rightW, propH);
            ImGui::EndChild();

            ImGui::InvisibleButton("##splitR", ImVec2(-1, 8));
            if (ImGui::IsItemActive())
            {
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

    void UserInterfaceEditor::DrawMainToolbar()
    {
        if (ImGui::Button("Nouveau")) { New(); }
        ImGui::SameLine();
        if (ImGui::Button("Ouvrir..."))
        {
            LoadFromFile("ui.qui");
        }
        ImGui::SameLine();
        if (ImGui::Button("Sauver"))
        {
            if (m_OpenedPath.empty()) SaveToFile("ui.qui");
            else SaveToFile(m_OpenedPath.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Sauver sous..."))
        {
            SaveToFile("ui.qui");
        }
        ImGui::SameLine(0, 24);
        if (ImGui::Button("Undo")) DoUndo();
        ImGui::SameLine();
        if (ImGui::Button("Redo")) DoRedo();

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

    void UserInterfaceEditor::DrawHierarchyPanel(float, float)
    {
        if (ImGui::Button("+ Button")) { AddElement(UIType::Button, ScreenToCanvas(ImGui::GetMousePos())); }
        ImGui::SameLine();
        if (ImGui::Button("+ Text")) { int idx = AddElement(UIType::Text, ScreenToCanvas(ImGui::GetMousePos())); if (idx >= 0) std::snprintf(m_Elements[idx].label, 128, "Text"); }
        ImGui::SameLine();
        if (ImGui::Button("+ Checkbox")) { AddElement(UIType::Checkbox, ScreenToCanvas(ImGui::GetMousePos())); }
        ImGui::SameLine();
        if (ImGui::Button("+ Progress")) { AddElement(UIType::ProgressBar, ScreenToCanvas(ImGui::GetMousePos())); }
        ImGui::SameLine();
        if (ImGui::Button("+ Image")) { AddElement(UIType::Image, ScreenToCanvas(ImGui::GetIO().MousePos)); }
        ImGui::SameLine();
        if (ImGui::Button("+ Slider")) { AddElement(UIType::Slider, ScreenToCanvas(ImGui::GetIO().MousePos)); }
        ImGui::SameLine();
        if (ImGui::Button("+ Input")) { AddElement(UIType::InputText, ScreenToCanvas(ImGui::GetIO().MousePos)); }


        ImGui::Separator();
        ImGui::Text("Elements (%zu)", m_Elements.size());
        ImGui::BeginChild("##list", ImVec2(-1, -1), false);

        for (int i = (int)m_Elements.size() - 1; i >= 0; --i)
        {
            auto& e = m_Elements[i];
            bool sel = (i == m_SelectedIndex);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (sel ? ImGuiTreeNodeFlags_Selected : 0);
            ImGui::TreeNodeEx((void*)(intptr_t)e.id, flags, "%s##%u", TypeToString(e.type), e.id);
            if (ImGui::IsItemClicked()) Select(i);

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("QUI_HIER", &i, sizeof(int));
                ImGui::Text("Deplacer: %s #%u", TypeToString(e.type), e.id);
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("QUI_HIER"))
                {
                    int src = *(const int*)p->Data;
                    if (src != i && src >= 0 && src < (int)m_Elements.size())
                    {
                        auto moved = m_Elements[src];
                        m_Elements.erase(m_Elements.begin() + src);
                        
                        int dstIndex = i + (src < i ? 0 : 1);
                        dstIndex = std::clamp(dstIndex, 0, (int)m_Elements.size());
                        m_Elements.insert(m_Elements.begin() + dstIndex, moved);
                        m_SelectedIndex = dstIndex;
                        PushUndo("Reorder");
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::EndChild();
    }

    void UserInterfaceEditor::DrawCanvasPanel(float, float)
    {
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

        ImGui::InvisibleButton("##CanvasIO", m_CanvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hovering = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        if (hovering)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Middle] = ImGui::GetIO().MousePos;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                m_Pan += ImGui::GetIO().MouseDelta;

            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                float prev = m_Zoom;
                m_Zoom = std::clamp(m_Zoom + wheel * 0.1f, 0.4f, 4.0f);
                ImVec2 mouse = ImGui::GetIO().MousePos;
                m_Pan = (m_Pan - mouse) * (m_Zoom / prev) + mouse;
            }
        }

        if (m_ShowGrid) DrawGrid(dl, m_CanvasPos, m_CanvasSize, m_GridStep * m_Zoom);

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("QUI_CanvasMenu");
        }
        if (ImGui::BeginPopup("QUI_CanvasMenu"))
        {
            ImVec2 mc = ScreenToCanvas(ImGui::GetIO().MousePos);
            if (ImGui::MenuItem("Ajouter Button")) AddElement(UIType::Button, mc);
            if (ImGui::MenuItem("Ajouter Text")) { int idx = AddElement(UIType::Text, mc); if (idx >= 0) std::snprintf(m_Elements[idx].label, 128, "Text"); }
            if (ImGui::MenuItem("Ajouter Checkbox")) AddElement(UIType::Checkbox, mc);
            if (ImGui::MenuItem("Ajouter ProgressBar")) AddElement(UIType::ProgressBar, mc);
            if (ImGui::MenuItem("Ajouter Separator")) AddElement(UIType::Separator, mc);
			if (ImGui::MenuItem("Ajouter Image")) AddElement(UIType::Image, mc);
			if (ImGui::MenuItem("Ajouter Slider")) AddElement(UIType::Slider, mc);
			if (ImGui::MenuItem("Ajouter InputText")) AddElement(UIType::InputText, mc);
            ImGui::Separator();
            if (ImGui::MenuItem("Dupliquer", nullptr, false, m_SelectedIndex >= 0)) DuplicateSelected();
            if (ImGui::MenuItem("Supprimer", nullptr, false, m_SelectedIndex >= 0)) DeleteSelected();
            ImGui::EndPopup();
        }

        {
            std::lock_guard<std::mutex> lk(m_Mutex);
            for (size_t i = 0; i < m_Elements.size(); ++i)
            {
                bool sel = ((int)i == m_SelectedIndex);
                DrawElement(dl, m_Elements[i], sel);
                if (sel) DrawResizeHandles(dl, m_Elements[i]);
            }
        }

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 mc = ScreenToCanvas(ImGui::GetIO().MousePos);
            int hit = -1, handle = -1;

            {
                std::lock_guard<std::mutex> lk(m_Mutex);
                for (int i = (int)m_Elements.size() - 1; i >= 0; --i)
                {
                    handle = HitTestHandle(m_Elements[i], ImGui::GetIO().MousePos);
                    if (handle >= 0) { hit = i; break; }
                    if (HitTest(m_Elements[i], mc)) { hit = i; break; }
                }
            }

            if (hit >= 0)
            {
                Select(hit);
                if (handle >= 0) StartResize(handle);
                else
                {
                    std::lock_guard<std::mutex> lk(m_Mutex);
                    StartDrag(mc, m_Elements[m_SelectedIndex]);
                }
            }
            else Select(-1);
        }

        if (active)
        {
            if (m_Dragging) ApplyDrag(ScreenToCanvas(ImGui::GetIO().MousePos));
            if (m_Resizing) ApplyResize(ScreenToCanvas(ImGui::GetIO().MousePos));
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (m_Dragging || m_Resizing) PushUndo("Transform");
            m_Dragging = false;
            m_Resizing = false;
            m_ResizeHandle = -1;
        }
    }

    void UserInterfaceEditor::DrawPropertiesPanel(float, float)
    {
        if (m_SelectedIndex < 0 || m_SelectedIndex >= (int)m_Elements.size())
        {
            ImGui::TextDisabled("Aucun element selectionne.");
            return;
        }

        auto& e = m_Elements[m_SelectedIndex];
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.45f, 1.f), "%s  (id=%u)", TypeToString(e.type), e.id);
        ImGui::Separator();

        bool changed = false;

        if (ImGui::Checkbox("Visible", &e.flags.visible)) changed = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Disabled", &e.flags.disabled)) changed = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("SameLine", &e.flags.sameLine)) changed = true;

        if (e.type == UIType::Text || e.type == UIType::Button || e.type == UIType::Checkbox)
        {
            int ah = (int)e.alignH;
            const char* AH[] = { "Gauche", "Centre", "Droite" };
            if (ImGui::Combo("Align. horizontal", &ah, "Gauche\0Centre\0Droite\0\0")) {
                e.alignH = (AlignH)ah;
                changed = true;
            }

            int av = (int)e.alignV;
            if (ImGui::Combo("Align. vertical", &av, "Haut\0Milieu\0Bas\0\0")) {
                e.alignV = (AlignV)av;
                changed = true;
            }

            if (ImGui::DragFloat("Police (px)", &e.fontPx, 0.5f, 0.0f, 128.0f, "%.1f")) {
                e.fontPx = std::max(0.0f, e.fontPx);
                changed = true;
            }
        }

        if (e.type != UIType::Separator)
        {
            char lbl[128]; std::strncpy(lbl, e.label, sizeof(lbl));
            if (ImGui::InputText("Label", lbl, IM_ARRAYSIZE(lbl)))
            {
                std::strncpy(e.label, lbl, sizeof(e.label));
                changed = true;
            }
        }

        if (e.type == UIType::Checkbox)
        {
            if (ImGui::Checkbox("Checked (preview default)", &e.checked)) changed = true;
        }
        else if (e.type == UIType::ProgressBar)
        {
            float f = e.fraction;
            if (ImGui::SliderFloat("Fraction", &f, 0.0f, 1.0f)) { e.fraction = Saturate(f); changed = true; }
            char ov[64]; std::strncpy(ov, e.overlay, sizeof(ov));
            if (ImGui::InputText("Overlay", ov, IM_ARRAYSIZE(ov)))
            {
                std::strncpy(e.overlay, ov, sizeof(e.overlay));
                changed = true;
            }
        }

        if (e.type != UIType::Separator)
        {
            ImGui::Separator();
            ImGui::Text("Rect (px)");
            float x = e.rect.x, y = e.rect.y, w = e.rect.w, h = e.rect.h;
            if (ImGui::DragFloat("X", &x, 1.f)) { e.rect.x = m_SnapToGrid ? Snap(x, m_GridStep) : x; changed = true; }
            if (ImGui::DragFloat("Y", &y, 1.f)) { e.rect.y = m_SnapToGrid ? Snap(y, m_GridStep) : y; changed = true; }
            if (ImGui::DragFloat("W", &w, 1.f, 2.f, 8192.f)) { e.rect.w = std::max(2.f, m_SnapToGrid ? Snap(w, m_GridStep) : w); changed = true; }
            if (ImGui::DragFloat("H", &h, 1.f, 2.f, 8192.f)) { e.rect.h = std::max(2.f, m_SnapToGrid ? Snap(h, m_GridStep) : h); changed = true; }
            ClampRect(e.rect);
        }

        if (e.type == UIType::Image)
        {
            bool changedLocal = false;
            char path[260]; std::strncpy(path, e.imagePath, sizeof(path));
            if (ImGui::InputText("Chemin image", path, IM_ARRAYSIZE(path))) {
                std::strncpy(e.imagePath, path, sizeof(e.imagePath));
                changedLocal = true;
            }
            ImGui::Checkbox("Conserver ratio", &e.imageKeepAspect);
            changedLocal = true ? changedLocal : false;

            if (ImGui::ColorEdit4("Tint", e.imageTint, ImGuiColorEditFlags_NoInputs)) {
                changedLocal = true;
            }

            /*if (Renderer::m_SceneData.m_AssetManager->isAssetLoaded("heightmap_preview"))
            {
                std::shared_ptr<Texture2D> tex = Renderer::m_SceneData.m_AssetManager->getAsset<Texture2D>("heightmap_preview");
                if (tex) {
                    ImGui::Image((ImTextureID)tex->GetHandle(), ImVec2(64, 64));
                }
            }*/

            if (changedLocal) changed = true;
        }

        if (e.type == UIType::Slider)
        {
            bool changedLocal = false;
            if (ImGui::DragFloat("Min", &e.sliderMin, 0.1f)) { changedLocal = true; }
            if (ImGui::DragFloat("Max", &e.sliderMax, 0.1f)) { if (e.sliderMax < e.sliderMin) e.sliderMax = e.sliderMin; changedLocal = true; }
            if (ImGui::SliderFloat("Valeur", &e.sliderValue, e.sliderMin, e.sliderMax)) { changedLocal = true; }
            char fmt[16]; std::strncpy(fmt, e.sliderFormat, sizeof(fmt));
            if (ImGui::InputText("Format", fmt, IM_ARRAYSIZE(fmt))) {
                std::strncpy(e.sliderFormat, fmt, sizeof(e.sliderFormat));
                changedLocal = true;
            }
            if (changedLocal) changed = true;
        }

        if (e.type == UIType::InputText)
        {
            bool changedLocal = false;

            char buf[256]; std::strncpy(buf, e.inputBuffer, sizeof(buf));
            if (ImGui::InputText("Texte (preview)", buf, IM_ARRAYSIZE(buf))) {
                std::strncpy(e.inputBuffer, buf, sizeof(e.inputBuffer));
                changedLocal = true;
            }

            uint32_t maxlen = e.inputMaxLen;
            if (ImGui::InputScalar("Max length", ImGuiDataType_U32, &maxlen)) {
                e.inputMaxLen = std::min<uint32_t>(maxlen, sizeof(e.inputBuffer));
                changedLocal = true;
            }

            if (ImGui::Checkbox("Mot de passe", &e.inputPassword)) changedLocal = true;
            if (ImGui::Checkbox("Multiligne", &e.inputMultiline))  changedLocal = true;

            if (changedLocal) changed = true;
        }

        if (changed) {  }
    }

    void UserInterfaceEditor::DrawPreviewPanel(float, float)
    {
        ImGui::Text("Previsualisation");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        if (!m_DesignW || !m_DesignH) {
            ImGui::TextDisabled("Design resolution invalide.");
            return;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginChild("##preview_frame", ImVec2(-1, -1), true, flags))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float rw = static_cast<float>(m_DesignW);
            float rh = static_cast<float>(m_DesignH);
            float scale = std::min(avail.x / rw, avail.y / rh);
            scale = std::max(0.1f, scale);

            ImVec2 previewSize = ImVec2(rw * scale, rh * scale);
            ImVec2 topLeft = ImVec2((avail.x - previewSize.x) * 0.5f,
                (avail.y - previewSize.y) * 0.5f);

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

    void UserInterfaceEditor::DrawGrid(ImDrawList* dl, ImVec2 origin, ImVec2 size, float step) const
    {
        ImU32 minor = IM_COL32(60, 62, 68, 120);
        for (float x = fmodf(m_Pan.x, step); x < size.x; x += step)
            dl->AddLine(origin + ImVec2(x, 0), origin + ImVec2(x, size.y), minor);
        for (float y = fmodf(m_Pan.y, step); y < size.y; y += step)
            dl->AddLine(origin + ImVec2(0, y), origin + ImVec2(size.x, y), minor);
    }

    void UserInterfaceEditor::DrawElement(ImDrawList* dl, const UIElement& e, bool selected) const
    {
        if (!e.flags.visible) {
            ImU32 col = IM_COL32(120, 120, 120, 100);
            ImVec2 p0 = CanvasToScreen(ImVec2(e.rect.x, e.rect.y));
            ImVec2 p1 = CanvasToScreen(ImVec2(e.rect.x + e.rect.w, e.rect.y + e.rect.h));
            dl->AddRect(p0, p1, col, 5.f, 0, 1.0f);
            return;
        }

        ImVec2 p0 = CanvasToScreen(ImVec2(e.rect.x, e.rect.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(e.rect.x + e.rect.w, e.rect.y + e.rect.h));
        ImU32 fill = selected ? IM_COL32(54, 132, 255, 40) : IM_COL32(100, 120, 140, 30);
        ImU32 border = selected ? IM_COL32(70, 160, 255, 255) : IM_COL32(160, 160, 170, 180);

        dl->AddRectFilled(p0, p1, fill, 6.f);
        dl->AddRect(p0, p1, border, 6.f, 0, 2.0f);

        char title[160] = {};
        std::snprintf(title, sizeof(title), "%s", (e.type == UIType::Separator) ? "Separator" : e.label);
        ImVec2 ts = ImGui::CalcTextSize(title);
        ImVec2 tc = (p0 + p1) * 0.5f - ts * 0.5f;
        dl->AddText(tc, IM_COL32(230, 230, 235, 230), title);
    }

    void UserInterfaceEditor::DrawResizeHandles(ImDrawList* dl, const UIElement& e) const
    {
        if (e.type == UIType::Separator) return;
        ImVec2 p0 = CanvasToScreen(ImVec2(e.rect.x, e.rect.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(e.rect.x + e.rect.w, e.rect.y + e.rect.h));
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

    bool UserInterfaceEditor::HitTest(const UIElement& e, ImVec2 p) const
    {
        if (e.type == UIType::Separator)
        {
            ImRect r(ImVec2(e.rect.x, e.rect.y - 4), ImVec2(e.rect.x + e.rect.w, e.rect.y + 4));
            return r.Contains(p);
        }
        ImRect r(ImVec2(e.rect.x, e.rect.y), ImVec2(e.rect.x + e.rect.w, e.rect.y + e.rect.h));
        return r.Contains(p);
    }

    int UserInterfaceEditor::HitTestHandle(const UIElement& e, ImVec2 screen) const
    {
        if (e.type == UIType::Separator) return -1;
        ImVec2 p0 = CanvasToScreen(ImVec2(e.rect.x, e.rect.y));
        ImVec2 p1 = CanvasToScreen(ImVec2(e.rect.x + e.rect.w, e.rect.y + e.rect.h));
        ImVec2 c = (p0 + p1) * 0.5f;

        ImVec2 pts[8] = {
            p0, ImVec2(c.x,p0.y), ImVec2(p1.x,p0.y),
            ImVec2(p1.x,c.y),
            p1, ImVec2(c.x,p1.y), ImVec2(p0.x,p1.y),
            ImVec2(p0.x,c.y)
        };
        for (int i = 0; i < 8; ++i)
        {
            ImRect r(pts[i] - ImVec2(m_HandleSize, m_HandleSize), pts[i] + ImVec2(m_HandleSize, m_HandleSize));
            if (r.Contains(screen)) return i;
        }
        return -1;
    }

    int UserInterfaceEditor::AddElement(UIType type, ImVec2 canvasPt)
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        UIElement e;
        e.id = m_NextId++;
        e.type = type;
        if (m_SnapToGrid) { canvasPt.x = Snap(canvasPt.x, m_GridStep); canvasPt.y = Snap(canvasPt.y, m_GridStep); }
        e.rect.x = canvasPt.x;
        e.rect.y = canvasPt.y;

        switch (type)
        {
        case UIType::Text:       e.rect.w = 140; e.rect.h = 28; std::snprintf(e.label, 128, "Text"); break;
        case UIType::Button:     e.rect.w = 120; e.rect.h = 36; std::snprintf(e.label, 128, "Button"); break;
        case UIType::Checkbox:   e.rect.w = 160; e.rect.h = 28; std::snprintf(e.label, 128, "Checkbox"); e.checked = false; break;
        case UIType::ProgressBar:e.rect.w = 200; e.rect.h = 20; e.fraction = 0.5f; e.overlay[0] = '\0'; break;
        case UIType::Separator:  e.rect.w = 220; e.rect.h = 6;  break;
        case UIType::Image:
            e.rect.w = 192; e.rect.h = 128;
            e.imagePath[0] = '\0';
            e.imageId[0] = '\0';
            e.imageKeepAspect = true;
            e.imageTint[0] = e.imageTint[1] = e.imageTint[2] = e.imageTint[3] = 1.0f;
            std::snprintf(e.label, 128, "Image");
            break;

        case UIType::Slider:
            e.rect.w = 220; e.rect.h = 26;
            e.sliderMin = 0.0f; e.sliderMax = 1.0f; e.sliderValue = 0.5f;
            std::snprintf(e.sliderFormat, sizeof(e.sliderFormat), "%%.2f");
            std::snprintf(e.label, 128, "Slider");
            break;

        case UIType::InputText:
            e.rect.w = 240; e.rect.h = 28;
            e.inputBuffer[0] = '\0';
            e.inputMaxLen = 256;
            e.inputPassword = false;
            e.inputMultiline = false;
            std::snprintf(e.label, 128, "Input");
            break;
        }

        m_Elements.push_back(e);
        m_SelectedIndex = (int)m_Elements.size() - 1;
        PushUndo("Add element");
        return m_SelectedIndex;
    }

    void UserInterfaceEditor::DeleteSelected()
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0 || m_SelectedIndex >= (int)m_Elements.size()) return;
        m_Elements.erase(m_Elements.begin() + m_SelectedIndex);
        m_SelectedIndex = -1;
        PushUndo("Delete");
    }

    void UserInterfaceEditor::DuplicateSelected()
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0 || m_SelectedIndex >= (int)m_Elements.size()) return;
        UIElement copy = m_Elements[m_SelectedIndex];
        copy.id = m_NextId++;
        copy.rect.x += 12.f; copy.rect.y += 12.f;
        m_Elements.push_back(copy);
        m_SelectedIndex = (int)m_Elements.size() - 1;
        PushUndo("Duplicate");
    }

    void UserInterfaceEditor::BringToFront()
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0) return;
        auto e = m_Elements[m_SelectedIndex];
        m_Elements.erase(m_Elements.begin() + m_SelectedIndex);
        m_Elements.push_back(e);
        m_SelectedIndex = (int)m_Elements.size() - 1;
        PushUndo("Z-Front");
    }

    void UserInterfaceEditor::SendToBack()
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0) return;
        auto e = m_Elements[m_SelectedIndex];
        m_Elements.erase(m_Elements.begin() + m_SelectedIndex);
        m_Elements.insert(m_Elements.begin(), e);
        m_SelectedIndex = 0;
        PushUndo("Z-Back");
    }

    void UserInterfaceEditor::Select(int idx)
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        m_SelectedIndex = (idx >= 0 && idx < (int)m_Elements.size()) ? idx : -1;
    }

    void UserInterfaceEditor::StartDrag(ImVec2 mouseCanvas, const UIElement& e)
    {
        m_Dragging = true;
        m_DragOffset = ImVec2(mouseCanvas.x - e.rect.x, mouseCanvas.y - e.rect.y);
    }

    void UserInterfaceEditor::StartResize(int handleIndex)
    {
        m_Resizing = true;
        m_ResizeHandle = handleIndex;
    }

    void UserInterfaceEditor::ApplyDrag(ImVec2 mouseCanvas)
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0) return;
        auto& e = m_Elements[m_SelectedIndex];
        float nx = mouseCanvas.x - m_DragOffset.x;
        float ny = mouseCanvas.y - m_DragOffset.y;
        if (m_SnapToGrid) { nx = Snap(nx, m_GridStep); ny = Snap(ny, m_GridStep); }
        e.rect.x = nx; e.rect.y = ny;
        ClampRect(e.rect);
    }

    void UserInterfaceEditor::ApplyResize(ImVec2 mouseCanvas)
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        if (m_SelectedIndex < 0 || m_ResizeHandle < 0) return;
        auto& r = m_Elements[m_SelectedIndex].rect;
        float minW = 12.f, minH = 8.f;

        switch (m_ResizeHandle)
        {
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

    void UserInterfaceEditor::PushUndo(const char*)
    {
        Snapshot s; s.elements = m_Elements; s.selected = m_SelectedIndex; s.nextId = m_NextId;
        m_Undo.push_back(std::move(s));
        m_Redo.clear();
    }
    void UserInterfaceEditor::DoUndo()
    {
        if (m_Undo.size() <= 1) return;
        Snapshot cur; cur.elements = m_Elements; cur.selected = m_SelectedIndex; cur.nextId = m_NextId;
        m_Redo.push_back(std::move(cur));

        auto prev = m_Undo[m_Undo.size() - 2];
        m_Elements = prev.elements; m_SelectedIndex = prev.selected; m_NextId = prev.nextId;
        m_Undo.pop_back();
    }
    void UserInterfaceEditor::DoRedo()
    {
        if (m_Redo.empty()) return;
        Snapshot next = m_Redo.back(); m_Redo.pop_back();
        Snapshot cur; cur.elements = m_Elements; cur.selected = m_SelectedIndex; cur.nextId = m_NextId;
        m_Undo.push_back(std::move(cur));
        m_Elements = next.elements; m_SelectedIndex = next.selected; m_NextId = next.nextId;
    }

    // ---------- Serialization (.qui binaire) ----------
    /*
        Format .qui (little-endian)
        magic[4] = 'Q','U','I','\0'
        version u16 = 1
        count   u32
        nextId  u32
        repeter count fois:
          id u32
          type u8
          flags u8  (bit0:visible, bit1:disabled, bit2:sameline)
          rect 4 * f32 (x,y,w,h)
          label[128] char
          checked u8
          fraction f32
          overlay[64] char
    */
    bool UserInterfaceEditor::Serialize(const char* path) const
    {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        char magic[4] = { 'Q','U','I','\0' };
        uint16_t ver = 1;
        uint32_t count = (uint32_t)m_Elements.size();
        uint32_t nextId = m_NextId;
        f.write(magic, 4);
        f.write((char*)&ver, sizeof(ver));
        f.write((char*)&count, sizeof(count));
        f.write((char*)&nextId, sizeof(nextId));
        for (const auto& e : m_Elements)
        {
            f.write((char*)&e.id, sizeof(e.id));
            uint8_t t = (uint8_t)e.type; f.write((char*)&t, 1);
            uint8_t fl = (e.flags.visible ? 1 : 0) | (e.flags.disabled ? 2 : 0) | (e.flags.sameLine ? 4 : 0);
            f.write((char*)&fl, 1);
            f.write((char*)&e.rect, sizeof(e.rect));
            f.write((char*)e.label, 128);
            uint8_t ch = e.checked ? 1 : 0; f.write((char*)&ch, 1);
            f.write((char*)&e.fraction, sizeof(e.fraction));
            f.write((char*)e.overlay, 64);
        }
        return (bool)f;
    }

    bool UserInterfaceEditor::Deserialize(const char* path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        char magic[4]; f.read(magic, 4);
        if (std::memcmp(magic, "QUI\0", 4) != 0) return false;
        uint16_t ver = 0; f.read((char*)&ver, sizeof(ver));
        if (ver != 1) return false;
        uint32_t count = 0, nextId = 1;
        f.read((char*)&count, sizeof(count));
        f.read((char*)&nextId, sizeof(nextId));
        std::vector<UIElement> tmp; tmp.resize(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            auto& e = tmp[i];
            f.read((char*)&e.id, sizeof(e.id));
            uint8_t t = 0; f.read((char*)&t, 1); e.type = (UIType)t;
            uint8_t fl = 0; f.read((char*)&fl, 1);
            e.flags.visible = (fl & 1) != 0;
            e.flags.disabled = (fl & 2) != 0;
            e.flags.sameLine = (fl & 4) != 0;
            f.read((char*)&e.rect, sizeof(e.rect));
            f.read((char*)e.label, 128);
            uint8_t ch = 0; f.read((char*)&ch, 1); e.checked = (ch != 0);
            f.read((char*)&e.fraction, sizeof(e.fraction));
            f.read((char*)e.overlay, 64);
        }
        m_Elements = std::move(tmp);
        m_NextId = nextId;
        m_SelectedIndex = m_Elements.empty() ? -1 : 0;
        return true;
    }

    ImVec2 UserInterfaceEditor::ScreenToCanvas(ImVec2 screen) const
    {
        ImVec2 p = screen - m_CanvasPos;
        return (p - m_Pan) / m_Zoom;
    }
    ImVec2 UserInterfaceEditor::CanvasToScreen(ImVec2 canvas) const
    {
        ImVec2 p = canvas * m_Zoom + m_Pan + m_CanvasPos;
        return p;
    }
    void UserInterfaceEditor::ClampRect(UIRect& r) const
    {
        r.w = std::max(2.f, r.w);
        r.h = std::max(2.f, r.h);
        r.x = std::max(-4096.f, std::min(4096.f, r.x));
        r.y = std::max(-4096.f, std::min(4096.f, r.y));
    }
    const char* UserInterfaceEditor::TypeToString(UIType t)
    {
        switch (t) {
        case UIType::Button:      return "Button";
        case UIType::Text:        return "Text";
        case UIType::Checkbox:    return "Checkbox";
        case UIType::ProgressBar: return "ProgressBar";
        case UIType::Separator:   return "Separator";
        case UIType::Image:       return "Image";
        case UIType::Slider:      return "Slider";
        case UIType::InputText:   return "InputText";
        }
        return "Unknown";
    }
    UserInterfaceEditor::UIType UserInterfaceEditor::StringToType(const char* s)
    {
        if (!s) return UIType::Button;
        if (std::strcmp(s, "Text") == 0) return UIType::Text;
        if (std::strcmp(s, "Checkbox") == 0) return UIType::Checkbox;
        if (std::strcmp(s, "ProgressBar") == 0) return UIType::ProgressBar;
        if (std::strcmp(s, "Separator") == 0) return UIType::Separator;
		if (std::strcmp(s, "Image") == 0) return UIType::Image;
		if (std::strcmp(s, "Slider") == 0) return UIType::Slider;
		if (std::strcmp(s, "InputText") == 0) return UIType::InputText;
        return UIType::Button;
    }

    ImVec2 UserInterfaceEditor::ComputeAlignedTextPos(const ImVec2& rectMin, const ImVec2& rectSize, const char* text, AlignH ah, AlignV av, float fontScale) const
    {
        const ImVec2 textSize = ImGui::CalcTextSize(text ? text : "", nullptr, false) * fontScale;

        float x = rectMin.x;
        float y = rectMin.y;

        if (ah == AlignH::Left)   x += 6.0f * fontScale;
        if (ah == AlignH::Center) x += (rectSize.x - textSize.x) * 0.5f;
        if (ah == AlignH::Right)  x += rectSize.x - textSize.x - 6.0f * fontScale;

        if (av == AlignV::Top)    y += 4.0f * fontScale;
        if (av == AlignV::Middle) y += (rectSize.y - textSize.y) * 0.5f;
        if (av == AlignV::Bottom) y += rectSize.y - textSize.y - 4.0f * fontScale;

        return ImVec2(x, y);
    }

    void UserInterfaceEditor::RenderRuntimePreview(float scale, ImVec2 offset) const
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 base = ImGui::GetCursorScreenPos() + offset;

        const ImU32 colButtonBG = IM_COL32(50, 60, 75, 255);
        const ImU32 colButtonBD = IM_COL32(90, 110, 140, 255);
        const ImU32 colText = IM_COL32(230, 230, 235, 255);
        const ImU32 colDisabled = IM_COL32(150, 150, 160, 180);
        const ImU32 colCheckboxBG = IM_COL32(40, 48, 60, 255);
        const ImU32 colCheckboxBD = IM_COL32(90, 110, 140, 255);
        const ImU32 colCheckMark = IM_COL32(240, 240, 245, 255);
        const ImU32 colProgressBG = IM_COL32(40, 46, 56, 255);
        const ImU32 colProgressFG = IM_COL32(90, 170, 240, 255);

        std::lock_guard<std::mutex> lk(m_Mutex);
        for (const auto& e : m_Elements)
        {
            if (!e.flags.visible) continue;

            ImVec2 rpos = base + ImVec2(e.rect.x * scale, e.rect.y * scale);
            ImVec2 rsz = ImVec2(e.rect.w * scale, e.rect.h * scale);
            ImVec2 rmin = rpos;
            ImVec2 rmax = rpos + rsz;

            const float currentPx = ImGui::GetFontSize();
            float desiredPx = (e.fontPx > 0.0f ? e.fontPx : currentPx);
            float fontScaleRel = desiredPx / currentPx;
            if (fontScaleRel <= 0.01f) fontScaleRel = 0.01f;

            ImU32 textColor = e.flags.disabled ? colDisabled : colText;

            switch (e.type)
            {
            case UIType::Text:
            {
                FontScaleScope fs(fontScaleRel);
                const char* txt = (e.label[0] ? e.label : "Text");
                ImVec2 tp = ComputeAlignedTextPos(rmin, rsz, txt, e.alignH, e.alignV, fontScaleRel);
                dl->AddText(tp, textColor, txt);
            } break;

            case UIType::Button:
            {
                dl->AddRectFilled(rmin, rmax, colButtonBG, 6.f);
                dl->AddRect(rmin, rmax, colButtonBD, 6.f, 0, 2.0f);

                FontScaleScope fs(fontScaleRel);
                const char* txt = (e.label[0] ? e.label : "Button");
                ImVec2 tp = ComputeAlignedTextPos(rmin, rsz, txt, e.alignH, e.alignV, fontScaleRel);
                dl->AddText(tp, textColor, txt);
            } break;

            case UIType::Checkbox:
            {
                float boxSide = ImClamp(rsz.y * 0.7f, 10.0f, 28.0f);
                ImVec2 boxSize(boxSide, boxSide);

                FontScaleScope fs(fontScaleRel);
                const char* txt = (e.label[0] ? e.label : "Checkbox");
                ImVec2 textSize = ImGui::CalcTextSize(txt) * fontScaleRel;
                float spacing = 8.0f * fontScaleRel;
                ImVec2 groupSize(boxSize.x + spacing + textSize.x, std::max(boxSize.y, textSize.y));

                ImVec2 gmin = rmin;
                
                if (e.alignH == AlignH::Left)   gmin.x += 6.0f * fontScaleRel;
                if (e.alignH == AlignH::Center) gmin.x += (rsz.x - groupSize.x) * 0.5f;
                if (e.alignH == AlignH::Right)  gmin.x += rsz.x - groupSize.x - 6.0f * fontScaleRel;
                
                if (e.alignV == AlignV::Top)    gmin.y += 4.0f * fontScaleRel;
                if (e.alignV == AlignV::Middle) gmin.y += (rsz.y - groupSize.y) * 0.5f;
                if (e.alignV == AlignV::Bottom) gmin.y += rsz.y - groupSize.y - 4.0f * fontScaleRel;

                ImVec2 c0 = gmin;
                ImVec2 c1 = gmin + boxSize;
                dl->AddRectFilled(c0, c1, colCheckboxBG, 3.f);
                dl->AddRect(c0, c1, colCheckboxBD, 3.f, 0, 1.5f);

                if (e.checked) {
                    ImVec2 a = ImVec2(c0.x + boxSide * 0.22f, c0.y + boxSide * 0.55f);
                    ImVec2 b = ImVec2(c0.x + boxSide * 0.42f, c0.y + boxSide * 0.75f);
                    ImVec2 c = ImVec2(c0.x + boxSide * 0.78f, c0.y + boxSide * 0.28f);
                    dl->AddLine(a, b, colCheckMark, 2.0f);
                    dl->AddLine(b, c, colCheckMark, 2.0f);
                }

                ImVec2 lp = ImVec2(gmin.x + boxSize.x + spacing, gmin.y + (groupSize.y - textSize.y) * 0.5f);
                dl->AddText(lp, textColor, txt);
            } break;

            case UIType::ProgressBar:
            {
                dl->AddRectFilled(rmin, rmax, colProgressBG, 4.f);
                
                float t = Saturate(e.fraction);
                ImVec2 barMax = ImVec2(rmin.x + rsz.x * t, rmax.y);
                dl->AddRectFilled(rmin, barMax, colProgressFG, 4.f);
                
                if (e.overlay[0]) {
                    FontScaleScope fs(fontScaleRel);
                    ImVec2 ts = ImGui::CalcTextSize(e.overlay) * fontScaleRel;
                    ImVec2 tp = ImVec2(rmin.x + (rsz.x - ts.x) * 0.5f, rmin.y + (rsz.y - ts.y) * 0.5f);
                    dl->AddText(tp, textColor, e.overlay);
                }
            } break;

            case UIType::Separator:
            {
                dl->AddLine(rmin, ImVec2(rmin.x + rsz.x, rmin.y), IM_COL32(160, 160, 160, 255), 1.0f);
            } break;

            case UIType::Image:
            {
                ImVec2 p = base + ImVec2(e.rect.x * scale, e.rect.y * scale);
                ImVec2 s = ImVec2(e.rect.w * scale, e.rect.h * scale);
                ImVec2 r0 = p, r1 = p + s;

                ImU32 col = IM_COL32((int)(e.imageTint[0] * 80 + 20), (int)(e.imageTint[1] * 80 + 20), (int)(e.imageTint[2] * 80 + 20), (int)(e.imageTint[3] * 255));
                dl->AddRectFilled(r0, r1, col, 4.f);
                dl->AddRect(r0, r1, IM_COL32(180, 180, 200, 200), 4.f, 0, 1.5f);

                const char* txt = (e.label[0] ? e.label : "Image");
                float currentPx = ImGui::GetFontSize();
                float desiredPx = (e.fontPx > 0.0f ? e.fontPx : currentPx);
                float fs = desiredPx / currentPx; if (fs < 0.01f) fs = 0.01f;
                FontScaleScope fss(fs);
                ImVec2 tp = ComputeAlignedTextPos(r0, s, txt, e.alignH, e.alignV, fs);
                dl->AddText(tp, IM_COL32(240, 240, 245, 220), txt);
            }
            break;

            case UIType::Slider:
            {
                ImVec2 p = base + ImVec2(e.rect.x * scale, e.rect.y * scale);
                ImVec2 s = ImVec2(e.rect.w * scale, e.rect.h * scale);
                ImVec2 r0 = p, r1 = p + s;

                ImU32 trackBG = IM_COL32(45, 50, 60, 255);
                ImU32 trackFG = IM_COL32(90, 170, 240, 255);
                dl->AddRectFilled(r0, r1, trackBG, 4.f);

                float t = 0.f;
                if (e.sliderMax > e.sliderMin)
                    t = (e.sliderValue - e.sliderMin) / (e.sliderMax - e.sliderMin);
                t = Saturate(t);

                ImVec2 f1 = ImVec2(r0.x + s.x * t, r1.y);
                dl->AddRectFilled(r0, ImVec2(f1.x, f1.y), trackFG, 4.f);

                float handleW = std::max(6.0f, s.y * 0.5f);
                float hx = r0.x + s.x * t;
                ImVec2 h0 = ImVec2(hx - handleW * 0.5f, r0.y);
                ImVec2 h1 = ImVec2(hx + handleW * 0.5f, r1.y);
                dl->AddRectFilled(h0, h1, IM_COL32(230, 230, 240, 255), 3.f);
                dl->AddRect(h0, h1, IM_COL32(40, 45, 55, 200), 3.f, 0, 1.0f);

                char valtxt[64];
                std::snprintf(valtxt, sizeof(valtxt), e.sliderFormat[0] ? e.sliderFormat : "%.2f", e.sliderValue);

                float currentPx = ImGui::GetFontSize();
                float desiredPx = (e.fontPx > 0.0f ? e.fontPx : currentPx);
                float fs = desiredPx / currentPx; if (fs < 0.01f) fs = 0.01f;
                FontScaleScope fss(fs);

                char txt[192];
                if (e.label[0]) std::snprintf(txt, sizeof(txt), "%s: %s", e.label, valtxt);
                else            std::snprintf(txt, sizeof(txt), "%s", valtxt);

                ImVec2 tp = ComputeAlignedTextPos(r0, s, txt, e.alignH, e.alignV, fs);
                dl->AddText(tp, IM_COL32(230, 230, 235, 255), txt);
            }
            break;

            case UIType::InputText:
            {
                ImVec2 p = base + ImVec2(e.rect.x * scale, e.rect.y * scale);
                ImVec2 s = ImVec2(e.rect.w * scale, e.rect.h * scale);
                ImVec2 r0 = p, r1 = p + s;

                dl->AddRectFilled(r0, r1, IM_COL32(35, 40, 50, 255), 4.f);
                dl->AddRect(r0, r1, IM_COL32(90, 110, 140, 255), 4.f, 0, 1.5f);

                const char* raw = (e.inputBuffer[0] ? e.inputBuffer : "");
                char shown[256];
                if (e.inputPassword) {
                    size_t n = std::min<size_t>(std::strlen(raw), sizeof(shown) - 1);
                    std::memset(shown, 0, sizeof(shown));
                    for (size_t i = 0; i < n; ++i) shown[i] = (char)0xE2;
                    std::snprintf(shown, sizeof(shown), "***");
                }
                else {
                    std::strncpy(shown, raw, sizeof(shown));
                }

                float currentPx = ImGui::GetFontSize();
                float desiredPx = (e.fontPx > 0.0f ? e.fontPx : currentPx);
                float fs = desiredPx / currentPx; if (fs < 0.01f) fs = 0.01f;
                FontScaleScope fss(fs);

                const char* txt = (e.label[0] ? e.label : shown);
                ImVec2 tp = ComputeAlignedTextPos(r0, s, txt, e.alignH, e.alignV, fs);
                dl->AddText(tp, IM_COL32(225, 225, 230, 255), txt);
            }
            break;
            }

            if (e.flags.sameLine) {
                
            }
        }
    }
}