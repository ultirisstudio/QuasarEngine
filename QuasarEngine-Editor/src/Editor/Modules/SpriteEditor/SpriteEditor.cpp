#include "SpriteEditor.h"

namespace QuasarEngine
{
    std::string SpriteEditor::ToProjectId(const std::filesystem::path& pIn)
    {
        std::filesystem::path p = pIn;
        if (p.is_absolute())
        {
            const auto root = AssetManager::Instance().getAssetPath();
            std::error_code ec;
            auto rel = std::filesystem::relative(p, root, ec);
            if (!ec && !rel.empty() && rel.native()[0] != '.')
                p = rel;
        }
        std::string s = p.generic_string();
        if (s.rfind("Assets/", 0) != 0 && s.rfind("Assets\\", 0) != 0)
            s = "Assets/" + s;
        return s;
    }

    ImTextureID SpriteEditor::GetImGuiTex(const std::string& projectId)
    {
        auto it = m_TexCache.find(projectId);
        if (it != m_TexCache.end()) return it->second;

        if (!projectId.empty() && !AssetManager::Instance().isAssetLoaded(projectId))
        {
            AssetToLoad a{}; a.id = projectId;
            a.path = AssetManager::Instance().ResolvePath(projectId).generic_string();
            a.type = AssetType::TEXTURE;
            AssetManager::Instance().loadAsset(a);
        }

        std::shared_ptr<Texture2D> tex = AssetManager::Instance().getAsset<Texture2D>(projectId);
        ImTextureID id = tex ? (ImTextureID)reinterpret_cast<void*>(static_cast<std::uintptr_t>(tex->GetHandle())) : 0;
        m_TexCache[projectId] = id;
        return id;
    }

    SpriteEditor::SpriteEditor()
    {
        AddLayer("Ground", 0);
        
        m_Brush.textureId.clear();
        m_Brush.uv = { 0,0,1,1 };
        m_Brush.tint = { 1,1,1,1 };
    }

    int SpriteEditor::AddLayer(const std::string& name, int sortingOrder)
    {
        Layer L;
        L.name = name;
        L.sortingOrder = sortingOrder;
        L.visible = true;
        L.cells.resize((size_t)m_GridW * (size_t)m_GridH);
        m_Layers.push_back(std::move(L));
        m_SelectedLayer = (int)m_Layers.size() - 1;
        return m_SelectedLayer;
    }

    void SpriteEditor::RemoveLayer(int index)
    {
        if (index < 0 || index >= (int)m_Layers.size()) return;

        Scene* scn = Renderer::Instance().GetScene();
        if (scn)
        {
            for (auto& c : m_Layers[index].cells)
            {
                if (c.entity != UUID::Null())
                {
                    if (auto eOpt = scn->GetEntityByUUID(c.entity))
						if (eOpt.has_value())
                            scn->DestroyEntity(eOpt.value().GetUUID());
                }
            }
        }
        m_Layers.erase(m_Layers.begin() + index);
        m_SelectedLayer = std::clamp(m_SelectedLayer, 0, (int)m_Layers.size() - 1);
    }

    void SpriteEditor::MoveLayer(int index, int delta)
    {
        if (index < 0 || index >= (int)m_Layers.size()) return;
        int newIndex = std::clamp(index + delta, 0, (int)m_Layers.size() - 1);
        if (newIndex == index) return;
        std::swap(m_Layers[index], m_Layers[newIndex]);
        m_SelectedLayer = newIndex;
    }

    void SpriteEditor::ResizeLayers()
    {
        for (auto& L : m_Layers)
            L.cells.resize((size_t)m_GridW * (size_t)m_GridH);
    }

    void SpriteEditor::PaintAt(Scene& scene, Layer& layer, int cx, int cy, const Brush& b)
    {
        if (cx < 0 || cy < 0 || cx >= m_GridW || cy >= m_GridH) return;
        const size_t idx = CellIndex(cx, cy);
        Cell& cell = layer.cells[idx];

        if (!b.textureId.empty() && !AssetManager::Instance().isAssetLoaded(b.textureId)) {
            AssetToLoad a{}; a.id = b.textureId;
            a.path = AssetManager::Instance().ResolvePath(b.textureId).generic_string();
            a.type = AssetType::TEXTURE;
            AssetManager::Instance().loadAsset(a);
        }

        if (cell.entity == UUID::Null())
        {
            Entity e = scene.CreateEntity("Tile_" + std::to_string(cx) + "_" + std::to_string(cy) + "_" + layer.name);
            e.AddComponent<TransformComponent>();
            e.AddComponent<SpriteComponent>();
            cell.entity = e.GetUUID();
        }

        auto eOpt = scene.GetEntityByUUID(cell.entity);
        if (!eOpt.has_value()) { cell = {}; return; }
        Entity e = *eOpt;

        auto& tr = e.GetComponent<TransformComponent>();
        tr.Position = {
            m_Origin.x + cx * m_CellW + m_CellW * 0.5f,
            m_Origin.y + cy * m_CellH + m_CellH * 0.5f,
            layer.sortingOrder * 0.001f
        };
        tr.Scale = { m_CellW, m_CellH, 1.0f };
        tr.Rotation = { 0,0,0 };

        auto& sp = e.GetComponent<SpriteComponent>();
        sp.GetSpecification().TextureId = b.textureId;
        sp.GetSpecification().Color = b.tint;

        float u0 = b.uv.x, v0 = b.uv.y;
        float u1 = b.uv.z, v1 = b.uv.w;
        glm::vec2 tiling = { u1 - u0, v1 - v0 };
        glm::vec2 offset = { u0, v0 };
        if (b.flipH) { tiling.x = -tiling.x; offset.x = u1; }
        if (b.flipV) { tiling.y = -tiling.y; offset.y = v1; }
        sp.GetSpecification().Tiling = tiling;
        sp.GetSpecification().Offset = offset;

        cell.textureId = b.textureId;
        cell.uv = b.uv;
        cell.flipH = b.flipH;
        cell.flipV = b.flipV;
        cell.tint = b.tint;
    }

    void SpriteEditor::EraseAt(Scene& scene, Layer& layer, int cx, int cy)
    {
        if (cx < 0 || cy < 0 || cx >= m_GridW || cy >= m_GridH) return;
        Cell& cell = layer.cells[CellIndex(cx, cy)];
        if (cell.entity != UUID::Null())
        {
            if (auto eOpt = scene.GetEntityByUUID(cell.entity))
                if (eOpt.has_value())
                    scene.DestroyEntity(eOpt.value().GetUUID());
        }
        cell = {};
    }

    void SpriteEditor::OnImGuiRender(const char* windowName)
    {
        ImGui::SetNextWindowSize(ImVec2(1280, 760), ImGuiCond_Once);
        ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoCollapse);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float leftRatio = 0.28f;
        static float rightRatio = 0.28f;
        float minLeft = 280.f, minRight = 320.f;
        float leftW = ImMax(avail.x * leftRatio, minLeft);
        float rightW = ImMax(avail.x * rightRatio, minRight);
        float centerW = ImMax(avail.x - leftW - rightW - 12.f, 420.f);

        ImGui::BeginChild("##Left", ImVec2(leftW, 0), true);
        DrawLeftPanel(leftW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##splitL", ImVec2(6, avail.y));
        if (ImGui::IsItemActive()) {
            leftRatio += ImGui::GetIO().MouseDelta.x / avail.x;
            leftRatio = std::clamp(leftRatio, 0.18f, 0.55f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Canvas", ImVec2(centerW, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        DrawCanvasPanel(centerW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##splitR", ImVec2(6, avail.y));
        if (ImGui::IsItemActive()) {
            rightRatio -= ImGui::GetIO().MouseDelta.x / avail.x;
            rightRatio = std::clamp(rightRatio, 0.20f, 0.6f);
        }
        ImGui::SameLine(0, 0);

        ImGui::BeginChild("##Right", ImVec2(rightW, 0), true);
        DrawRightPanel(rightW, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();

        ImGui::End();
    }

    void SpriteEditor::DrawLeftPanel(float, float)
    {
        if (ImGui::Button("Ajouter calque")) {
            int so = (int)m_Layers.size();
            AddLayer("Layer " + std::to_string(so), so);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::DragInt("W", &m_GridW, 1.f, 1, 512);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::DragInt("H", &m_GridH, 1.f, 1, 512);
        if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemDeactivatedAfterEdit()) ResizeLayers();

        ImGui::SetNextItemWidth(120);
        if (ImGui::DragFloat("Cell W", &m_CellW, 1.f, 4.f, 1024.f)) m_CellW = std::max(4.f, m_CellW);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::DragFloat("Cell H", &m_CellH, 1.f, 4.f, 1024.f)) m_CellH = std::max(4.f, m_CellH);

        ImGui::Separator();
        
        ImGui::Text("Calques (%d)", (int)m_Layers.size());
        for (int i = (int)m_Layers.size() - 1; i >= 0; --i)
        {
            ImGui::PushID(i);
            bool sel = (i == m_SelectedLayer);
            ImGui::Selectable(m_Layers[i].name.c_str(), sel);
            if (ImGui::IsItemClicked()) m_SelectedLayer = i;

            ImGui::SameLine();
            ImGui::Checkbox("##vis", &m_Layers[i].visible);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::DragInt("Z", &m_Layers[i].sortingOrder, 1.f, -1000, 1000);

            if (ImGui::BeginPopupContextItem("LayerCtx"))
            {
                if (ImGui::MenuItem("Monter")) MoveLayer(i, +1);
                if (ImGui::MenuItem("Descendre")) MoveLayer(i, -1);
                ImGui::Separator();
                if (ImGui::MenuItem("Renommer")) {
                    
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Supprimer")) { RemoveLayer(i); ImGui::EndPopup(); ImGui::PopID(); break; }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        DrawTexturePalette();
    }

    void SpriteEditor::DrawTexturePalette()
    {
        ImGui::Text("Palette textures");
        static char filter[128] = "";
        ImGui::SetNextItemWidth(180);
        ImGui::InputTextWithHint("##filter", "filtre...", filter, IM_ARRAYSIZE(filter));
        ImGui::SameLine();
        ImGui::TextDisabled("Glissez des fichiers depuis le Content Browser ici");

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                const wchar_t* wpath = (const wchar_t*)p->Data;
                std::filesystem::path dropped(wpath);
                std::string id = ToProjectId(dropped);
                if (std::find(m_Palette.begin(), m_Palette.end(), id) == m_Palette.end())
                    m_Palette.push_back(id);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::Dummy(ImVec2(0, 4));
        const float thumb = 72.f;
        const float pad = 8.f;
        float avail = ImGui::GetContentRegionAvail().x;
        int cols = std::max(1, (int)std::floor((avail + pad) / (thumb + pad)));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(pad, pad));
        int col = 0;

        for (size_t i = 0; i < m_Palette.size(); ++i)
        {
            const std::string& id = m_Palette[i];
            if (filter[0] && id.find(filter) == std::string::npos) continue;

            if (col > 0) ImGui::SameLine();
            ImGui::BeginGroup();
            ImTextureID t = GetImGuiTex(id);
            ImVec2 size(thumb, thumb);

            ImGui::PushID((int)i);
            if (t) {
                if (ImGui::ImageButton("##tex", t, size, ImVec2(0, 1), ImVec2(1, 0))) {
                    m_Brush.textureId = id;
                    m_Brush.uv = { 0,0,1,1 };
                    m_Brush.flipH = m_Brush.flipV = false;
                    m_Brush.tint = { 1,1,1,1 };
                }
            }
            else {
                ImGui::Button("no tex", size);
            }

            if (ImGui::IsItemHovered() && t) {
                ImGui::BeginTooltip();
                ImGui::Image(t, ImVec2(thumb * 2, thumb * 2), ImVec2(0, 1), ImVec2(1, 0));
                ImGui::TextUnformatted(id.c_str());
                ImGui::EndTooltip();
            }

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("SPRITE_PALETTE_ID", id.c_str(), id.size() + 1);
                if (t) ImGui::Image(t, ImVec2(48, 48), ImVec2(0, 1), ImVec2(1, 0));
                ImGui::TextUnformatted(id.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + thumb);
            ImGui::TextUnformatted(std::filesystem::path(id).filename().string().c_str());
            ImGui::PopTextWrapPos();

            ImGui::EndGroup();
            if (++col >= cols) { col = 0; }
        }

        ImGui::PopStyleVar();
    }

    void SpriteEditor::DrawGrid(ImDrawList* dl, ImVec2 origin, ImVec2 size, float step) const
    {
        ImU32 minor = IM_COL32(60, 62, 68, 120);
        for (float x = fmodf(m_Pan.x, step); x < size.x; x += step)
            dl->AddLine(origin + ImVec2(x, 0), origin + ImVec2(x, size.y), minor);
        for (float y = fmodf(m_Pan.y, step); y < size.y; y += step)
            dl->AddLine(origin + ImVec2(0, y), origin + ImVec2(size.x, y), minor);
    }

    void SpriteEditor::DrawPlacedSpritesOnCanvas(ImDrawList* dl, const Layer& layer)
    {
        if (!layer.visible) return;

        for (int y = 0; y < m_GridH; ++y)
            for (int x = 0; x < m_GridW; ++x)
            {
                const Cell& c = layer.cells[CellIndex(x, y)];
                if (c.textureId.empty()) continue;

                ImTextureID tex = GetImGuiTex(c.textureId);
                if (!tex) continue;

                ImVec2 c0 = CanvasToScreen(ImVec2(m_Origin.x + x * m_CellW, m_Origin.y + y * m_CellH));
                ImVec2 c1 = CanvasToScreen(ImVec2(m_Origin.x + (x + 1) * m_CellW, m_Origin.y + (y + 1) * m_CellH));

                ImVec2 uv0, uv1;
                GLRectToImGuiUV(c.uv.x, c.uv.y, c.uv.z, c.uv.w, uv0, uv1);

                ImU32 tint = ImGui::GetColorU32(ImVec4(c.tint.r, c.tint.g, c.tint.b, c.tint.a));
                dl->AddImage(tex, c0, c1, uv0, uv1, tint);
            }

        if (m_HoverValid && !m_Brush.textureId.empty())
        {
            ImTextureID t = GetImGuiTex(m_Brush.textureId);
            if (t)
            {
                ImVec2 c0 = CanvasToScreen(ImVec2(m_Origin.x + m_HoverX * m_CellW, m_Origin.y + m_HoverY * m_CellH));
                ImVec2 c1 = CanvasToScreen(ImVec2(m_Origin.x + (m_HoverX + 1) * m_CellW, m_Origin.y + (m_HoverY + 1) * m_CellH));

                ImVec2 uv0, uv1; GLRectToImGuiUV(m_Brush.uv.x, m_Brush.uv.y, m_Brush.uv.z, m_Brush.uv.w, uv0, uv1);
                ImU32 tint = ImGui::GetColorU32(ImVec4(m_Brush.tint.r, m_Brush.tint.g, m_Brush.tint.b, 0.35f));
                dl->AddImage(t, c0, c1, uv0, uv1, tint);
            }
        }
    }

    void SpriteEditor::DrawCanvasPanel(float, float)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        m_CanvasPos = ImGui::GetCursorScreenPos();
        m_CanvasSize = ImGui::GetContentRegionAvail();

        dl->AddRectFilled(m_CanvasPos, m_CanvasPos + m_CanvasSize, IM_COL32(22, 24, 28, 255), 6.0f);
        dl->AddRect(m_CanvasPos, m_CanvasPos + m_CanvasSize, IM_COL32(90, 90, 100, 180), 6.0f);

        ImGui::InvisibleButton("##CanvasIO", m_CanvasSize,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle);
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

        ImU32 gridCol = IM_COL32(70, 72, 78, 120);
        for (int x = 0; x <= m_GridW; ++x) {
            ImVec2 a = CanvasToScreen(ImVec2(m_Origin.x + x * m_CellW, m_Origin.y));
            ImVec2 b = CanvasToScreen(ImVec2(m_Origin.x + x * m_CellW, m_Origin.y + m_GridH * m_CellH));
            dl->AddLine(a, b, gridCol);
        }
        for (int y = 0; y <= m_GridH; ++y) {
            ImVec2 a = CanvasToScreen(ImVec2(m_Origin.x, m_Origin.y + y * m_CellH));
            ImVec2 b = CanvasToScreen(ImVec2(m_Origin.x + m_GridW * m_CellW, m_Origin.y + y * m_CellH));
            dl->AddLine(a, b, gridCol);
        }

        for (const auto& L : m_Layers) DrawPlacedSpritesOnCanvas(dl, L);

        ImVec2 mc = ScreenToCanvas(ImGui::GetIO().MousePos);
        m_HoverX = (int)std::floor((mc.x - m_Origin.x) / m_CellW);
        m_HoverY = (int)std::floor((mc.y - m_Origin.y) / m_CellH);
        m_HoverValid = (m_HoverX >= 0 && m_HoverY >= 0 && m_HoverX < m_GridW && m_HoverY < m_GridH);

        if (m_HoverValid)
        {
            ImVec2 c0 = CanvasToScreen(ImVec2(m_Origin.x + m_HoverX * m_CellW, m_Origin.y + m_HoverY * m_CellH));
            ImVec2 c1 = CanvasToScreen(ImVec2(m_Origin.x + (m_HoverX + 1) * m_CellW, m_Origin.y + (m_HoverY + 1) * m_CellH));
            dl->AddRect(c0, c1, IM_COL32(255, 230, 140, 220), 0.f, 0, 2.0f);
        }

        Scene* scn = Renderer::Instance().GetScene();
        if (scn && m_SelectedLayer >= 0 && m_SelectedLayer < (int)m_Layers.size())
        {
            Layer& L = m_Layers[m_SelectedLayer];
            if (m_HoverValid)
            {
                if (ImGui::GetIO().KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const Cell& c = L.cells[CellIndex(m_HoverX, m_HoverY)];
                    if (!c.textureId.empty()) {
                        m_Brush.textureId = c.textureId;
                        m_Brush.uv = c.uv;
                        m_Brush.flipH = c.flipH;
                        m_Brush.flipV = c.flipV;
                        m_Brush.tint = c.tint;
                    }
                }
                
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_Brush.textureId.empty()) {
                    PaintAt(*scn, L, m_HoverX, m_HoverY, m_Brush);
                    m_SelectedCellX = m_HoverX;
                    m_SelectedCellY = m_HoverY;
                }
                
                if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                    EraseAt(*scn, L, m_HoverX, m_HoverY);
                    if (m_SelectedCellX == m_HoverX && m_SelectedCellY == m_HoverY) {
                        m_SelectedCellX = m_SelectedCellY = -1;
                    }
                }
                
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
                    m_SelectedCellX = m_HoverX; m_SelectedCellY = m_HoverY;
                }
            }
        }
    }

    void SpriteEditor::DrawRightPanel(float, float)
    {
        ImGui::Text("Pinceau");
        ImGui::Separator();

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                const wchar_t* wpath = (const wchar_t*)p->Data;
                std::filesystem::path dropped(wpath);
                m_Brush.textureId = ToProjectId(dropped);
                m_Brush.uv = { 0,0,1,1 };
                m_Brush.flipH = m_Brush.flipV = false;
                m_Brush.tint = { 1,1,1,1 };
            }
            ImGui::EndDragDropTarget();
        }

        if (!m_Brush.textureId.empty()) {
            ImTextureID t = GetImGuiTex(m_Brush.textureId);
            ImGui::TextUnformatted(m_Brush.textureId.c_str());
            if (t) {
                ImVec2 sz(220, 220);
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::Image(t, sz, ImVec2(0, 1), ImVec2(1, 0));
                
                ImVec2 uv0, uv1; GLRectToImGuiUV(m_Brush.uv.x, m_Brush.uv.y, m_Brush.uv.z, m_Brush.uv.w, uv0, uv1);
                ImVec2 r0 = p0 + ImVec2(uv0.x * sz.x, uv0.y * sz.y);
                ImVec2 r1 = p0 + ImVec2(uv1.x * sz.x, uv1.y * sz.y);
                auto dl = ImGui::GetWindowDrawList();
                dl->AddRect(r0, r1, IM_COL32(255, 90, 90, 255), 0.f, 0, 2.0f);
                ImGui::Dummy(sz);
            }
        }
        else {
            ImGui::TextDisabled("Aucune texture selectionnee.");
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Text("UV (u0,v0,u1,v1)");
        ImGui::SetNextItemWidth(220);
        ImGui::DragFloat4("##uv", &m_Brush.uv.x, 0.001f, 0.f, 1.f, "%.3f");
        m_Brush.uv.x = std::clamp(m_Brush.uv.x, 0.f, 1.f);
        m_Brush.uv.y = std::clamp(m_Brush.uv.y, 0.f, 1.f);
        m_Brush.uv.z = std::clamp(m_Brush.uv.z, 0.f, 1.f);
        m_Brush.uv.w = std::clamp(m_Brush.uv.w, 0.f, 1.f);

        ImGui::Checkbox("Flip H", &m_Brush.flipH);
        ImGui::SameLine();
        ImGui::Checkbox("Flip V", &m_Brush.flipV);

        ImGui::ColorEdit4("Tint", &m_Brush.tint.x, ImGuiColorEditFlags_NoInputs);

        ImGui::Separator();

        ImGui::Text("Cellule selectionnee");
        if (m_SelectedCellX >= 0 && m_SelectedCellY >= 0 &&
            m_SelectedLayer >= 0 && m_SelectedLayer < (int)m_Layers.size())
        {
            Layer& L = m_Layers[m_SelectedLayer];
            Cell& c = L.cells[CellIndex(m_SelectedCellX, m_SelectedCellY)];

            ImGui::Text("Case: (%d, %d)  Layer: %s", m_SelectedCellX, m_SelectedCellY, L.name.c_str());
            if (!c.textureId.empty())
            {
                ImGui::TextUnformatted(c.textureId.c_str());
                ImTextureID t = GetImGuiTex(c.textureId);
                if (t) {
                    ImVec2 sz(220, 220);
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImGui::Image(t, sz, ImVec2(0, 1), ImVec2(1, 0));
                    ImVec2 uv0, uv1; GLRectToImGuiUV(c.uv.x, c.uv.y, c.uv.z, c.uv.w, uv0, uv1);
                    ImVec2 r0 = p0 + ImVec2(uv0.x * sz.x, uv0.y * sz.y);
                    ImVec2 r1 = p0 + ImVec2(uv1.x * sz.x, uv1.y * sz.y);
                    auto dl = ImGui::GetWindowDrawList();
                    dl->AddRect(r0, r1, IM_COL32(255, 160, 60, 255), 0.f, 0, 2.0f);
                    ImGui::Dummy(sz);
                }

                if (ImGui::Button("Appliquer pinceau a cette case")) {
                    Scene* scn = Renderer::Instance().GetScene();
                    if (scn) PaintAt(*scn, L, m_SelectedCellX, m_SelectedCellY, m_Brush);
                }
                ImGui::SameLine();
                if (ImGui::Button("Gommer cette case")) {
                    Scene* scn = Renderer::Instance().GetScene();
                    if (scn) EraseAt(*scn, L, m_SelectedCellX, m_SelectedCellY);
                }
            }
            else
            {
                ImGui::TextDisabled("Case vide.");
                if (ImGui::Button("Peindre (pinceau)")) {
                    Scene* scn = Renderer::Instance().GetScene();
                    if (scn && !m_Brush.textureId.empty())
                        PaintAt(*scn, L, m_SelectedCellX, m_SelectedCellY, m_Brush);
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Aucune case selectionnee.");
        }

        ImGui::Separator();
        DrawPreviewPanel(0, 0);
    }

    void SpriteEditor::DrawPreviewPanel(float, float)
    {
        ImGui::Text("Previsualisation");
        ImGui::Dummy(ImVec2(0, 4));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginChild("##preview_frame", ImVec2(-1, 260), true, flags))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float rw = m_GridW * m_CellW;
            float rh = m_GridH * m_CellH;
            if (rw <= 0 || rh <= 0) { ImGui::EndChild(); return; }

            float scale = std::min(avail.x / rw, avail.y / rh);
            scale = std::max(0.1f, scale);
            ImVec2 size(rw * scale, rh * scale);
            ImVec2 topLeft((avail.x - size.x) * 0.5f, (avail.y - size.y) * 0.5f);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 base = ImGui::GetCursorScreenPos();
            ImVec2 p0 = base + topLeft;
            ImVec2 p1 = p0 + size;

            dl->AddRectFilled(p0, p1, IM_COL32(18, 20, 24, 255), 6.f);
            ImGui::PushClipRect(p0, p1, true);

            for (const auto& L : m_Layers)
            {
                if (!L.visible) continue;
                for (int y = 0; y < m_GridH; ++y)
                    for (int x = 0; x < m_GridW; ++x)
                    {
                        const Cell& c = L.cells[CellIndex(x, y)];
                        if (c.textureId.empty()) continue;

                        ImTextureID tex = GetImGuiTex(c.textureId);
                        if (!tex) continue;

                        ImVec2 q0 = p0 + ImVec2(x * m_CellW * scale, y * m_CellH * scale);
                        ImVec2 q1 = q0 + ImVec2(m_CellW * scale, m_CellH * scale);

                        ImVec2 uv0, uv1; GLRectToImGuiUV(c.uv.x, c.uv.y, c.uv.z, c.uv.w, uv0, uv1);
                        ImU32 tint = ImGui::GetColorU32(ImVec4(c.tint.r, c.tint.g, c.tint.b, c.tint.a));
                        dl->AddImage(tex, q0, q1, uv0, uv1, tint);
                    }
            }

            ImGui::PopClipRect();
        }
        ImGui::EndChild();
    }
}
