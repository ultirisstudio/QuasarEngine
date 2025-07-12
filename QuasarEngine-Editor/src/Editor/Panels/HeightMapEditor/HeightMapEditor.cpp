#include "HeightMapEditor.h"

#include "imgui/imgui_internal.h"
#include "QuasarEngine/Renderer/Renderer.h"

namespace QuasarEngine
{
    HeightMapEditor::HeightMapEditor()
    {
        m_Curve.AddPoint({ 0.05f, 200.f });
        m_Curve.AddPoint({ 0.2f, 140.f });
        m_Curve.AddPoint({ 0.4f, 110.f });
        m_Curve.AddPoint({ 0.65f, 120.f });
        m_Curve.AddPoint({ 0.95f, 210.f });

        m_NeedRegen = false;
        m_WorkerBusy = false;
        m_NewImageReady = false;
        m_WorkerStop = false;
        m_Dragging = false;
        m_LastUpdateTime = 0.0;
        m_LastDirtyTime = 0.0;
        m_RefreshInterval = 0.07f;

        StartWorker();

        GenerateImage();
        RefreshTexture();

        TextureSpecification spec;
        spec.width = m_Params.width;
        spec.height = m_Params.height;
        spec.channels = m_Params.channels;
        spec.compressed = false;

        AssetToLoad asset;
        asset.type = AssetType::TEXTURE;
        asset.id = "heightmap_preview";
        asset.data = m_ImageData.data();
        asset.size = m_ImageData.size();
        asset.spec = spec;

        Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
    }

    HeightMapEditor::~HeightMapEditor()
    {
        StopWorker();

        m_Texture.reset();
    }

    void HeightMapEditor::OnImGuiRender(const char* windowName)
    {
        ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_Once);
        ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float left_width_ratio = 0.52f;
        static float preview_height_ratio = 0.62f;
        float min_panel_width = 240.f;
        float min_panel_height = 180.f;
        float left_panel_width = ImMax(avail.x * left_width_ratio, min_panel_width);

        ImGui::BeginChild("##CurveEditorPanel", ImVec2(left_panel_width, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImVec2 child_size = ImGui::GetContentRegionAvail();
            ImVec2 curveSize = ImVec2(ImMax(child_size.x - 10.f, 340.f), ImMax(child_size.y - 10.f, 260.f));
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 origin = ImGui::GetCursorScreenPos();
            DrawCurveEditor(draw_list, origin, curveSize);
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##splitter_v", ImVec2(8, avail.y));
        if (ImGui::IsItemActive())
        {
            left_width_ratio += ImGui::GetIO().MouseDelta.x / avail.x;
            left_width_ratio = glm::clamp(left_width_ratio, 0.22f, 0.78f);
        }
        ImGui::SameLine(0, 0);
        float right_panel_width = avail.x - left_panel_width - 8.0f;

        ImGui::BeginChild("##RightPanel", ImVec2(right_panel_width, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImVec2 right_avail = ImGui::GetContentRegionAvail();
            float preview_panel_height = ImMax(right_avail.y * preview_height_ratio, min_panel_height);

            ImGui::BeginChild("##PreviewPanel", ImVec2(-1, preview_panel_height), true);
            {
                ImGui::Text("Prévisualisation de la height map");
                ImGui::Separator();
                DrawImagePreview();
                ImGui::Dummy(ImVec2(0, 8));
            }
            ImGui::EndChild();

            ImGui::InvisibleButton("##splitter_h", ImVec2(-1, 8));
            if (ImGui::IsItemActive())
            {
                preview_height_ratio += ImGui::GetIO().MouseDelta.y / right_avail.y;
                preview_height_ratio = glm::clamp(preview_height_ratio, 0.22f, 0.85f);
            }

            ImGui::BeginChild("##ParamsPanel", ImVec2(-1, -1), true);
            {
                ImGui::Text("Paramètres de la height map");
                ImGui::Separator();
                bool changed = false;
                changed |= ImGui::InputInt("Largeur", (int*)&m_Params.width);
                changed |= ImGui::InputInt("Hauteur", (int*)&m_Params.height);
                changed |= ImGui::SliderInt("Octaves", &m_Params.octaves, 1, 10);
                changed |= ImGui::SliderFloat("Echelle", &m_Params.scale, 0.001f, 0.1f, "%.3f");
                changed |= ImGui::InputInt("Seed", (int*)&m_Params.seed);
                if (changed && !m_WorkerBusy)
                    m_NeedRegen = true;

                ImGui::Spacing();
                if (ImGui::Button("Ajouter point"))
                {
                    m_Curve.AddPoint({ 0.5f, 128.f });
                    if (!m_WorkerBusy)
                        m_NeedRegen = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Supprimer dernier") && m_Curve.PointCount() > 2)
                {
                    m_Curve.RemovePoint(m_Curve.PointCount() - 1);
                    if (!m_WorkerBusy)
                        m_NeedRegen = true;
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::End();

        ImGuiIO& io = ImGui::GetIO();
        double now = io.DeltaTime > 0.0 ? ImGui::GetTime() : m_LastUpdateTime;

        if (m_NewImageReady)
        {
            std::lock_guard<std::mutex> lock(m_ImageMutex);
            m_ImageData = m_WorkerImageData;
            RefreshTexture();
            m_NewImageReady = false;
            m_ImageDirty = false;
        }
        m_LastUpdateTime = now;
    }

    void HeightMapEditor::Update()
    {
        
    }

    void HeightMapEditor::DrawCurveEditor(ImDrawList* draw_list, const ImVec2& origin, const ImVec2& size)
    {
        static double lastDragRegenTime = 0.0;
        ImGuiIO& io = ImGui::GetIO();
        double now = io.DeltaTime > 0.0 ? ImGui::GetTime() : 0.0;

        ImU32 border_col = IM_COL32(180, 180, 180, 255);
        draw_list->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), border_col, 6.0f);

        auto& pts = m_Curve.GetPoints();
        std::vector<glm::vec2> sorted = m_Curve.GetSortedPoints();
        std::vector<glm::vec2> interp = m_Curve.InterpolateCurve(sorted, 128);

        for (size_t i = 1; i < interp.size(); ++i)
        {
            ImVec2 p0 = { origin.x + interp[i - 1].x * size.x, origin.y + (1.0f - interp[i - 1].y / 255.f) * size.y };
            ImVec2 p1 = { origin.x + interp[i].x * size.x, origin.y + (1.0f - interp[i].y / 255.f) * size.y };
            draw_list->AddLine(p0, p1, IM_COL32(40, 200, 90, 255), 2.0f);
        }

        bool any_dragging = false;

        for (size_t i = 0; i < pts.size(); ++i)
        {
            auto& pt = pts[i];
            ImVec2 p = { origin.x + pt.x * size.x, origin.y + (1.0f - pt.y / 255.f) * size.y };
            float rad = 6.0f;
            draw_list->AddCircleFilled(p, rad, IM_COL32(250, 70, 70, 255));

            ImGui::SetCursorScreenPos({ p.x - rad, p.y - rad });
            ImGui::InvisibleButton(("pt" + std::to_string(i)).c_str(), ImVec2(rad * 2, rad * 2));

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;

                float new_x = (p.x + delta.x - origin.x) / size.x;
                float new_y = 1.0f - (p.y + delta.y - origin.y) / size.y;

                new_x = glm::clamp(new_x, 0.f, 1.f);
                new_y = glm::clamp(new_y * 255.f, 0.f, 255.f);

                m_Curve.MovePoint(i, { new_x, new_y });

                any_dragging = true;
                if (!m_WorkerBusy && (now - lastDragRegenTime > 0.07))
                {
                    m_NeedRegen = true;
                    lastDragRegenTime = now;
                }
            }
            else if (ImGui::IsItemDeactivated())
            {
                m_Dragging = false;
                if (!m_WorkerBusy)
                    m_NeedRegen = true;
            }
        }
        if (!any_dragging && m_Dragging)
        {
            if (!m_WorkerBusy)
                m_NeedRegen = true;
        }
        m_Dragging = any_dragging;
    }

    void HeightMapEditor::DrawImagePreview()
	{
        if (Renderer::m_SceneData.m_AssetManager->isAssetLoaded("heightmap_preview"))
        {
            m_Texture = Renderer::m_SceneData.m_AssetManager->getAsset<Texture2D>("heightmap_preview");
            if (!m_Texture) return;
            ImVec2 size = { m_Params.width / 2.f, m_Params.height / 2.f };
            ImGui::Image((ImTextureID)m_Texture->GetHandle(), size);
        }
    }

    void HeightMapEditor::RefreshTexture()
	{
        TextureSpecification spec;
        spec.width = m_Params.width;
        spec.height = m_Params.height;
        spec.channels = m_Params.channels;
        spec.compressed = false;

        AssetToLoad asset;
        asset.type = AssetType::TEXTURE;
        asset.id = "heightmap_preview";
        asset.data = m_ImageData.data();
        asset.size = m_ImageData.size();
        asset.spec = spec;

        Renderer::m_SceneData.m_AssetManager->updateAsset(asset);
    }

    void HeightMapEditor::GenerateImage() {
        m_ImageData.clear();
        m_ImageData.resize(m_Params.width * m_Params.height * m_Params.channels);

        siv::PerlinNoise perlin(m_Params.seed);

        auto points = m_Curve.GetSortedPoints();

        for (uint32_t y = 0; y < m_Params.height; ++y) {
            for (uint32_t x = 0; x < m_Params.width; ++x) {
                float nx = float(x) / float(m_Params.width - 1);
                float ny = float(y) / float(m_Params.height - 1);

                double n = perlin.octave2D(nx / m_Params.scale, ny / m_Params.scale, m_Params.octaves);
                n = glm::clamp(float(n), -1.f, 1.f);
                
                float fn = static_cast<float>(n);
                float curve_x = glm::clamp((fn + 1.f) * 0.5f, 0.0f, 1.0f);
                float h = m_Curve.Evaluate(curve_x, points);

                uint8_t v = static_cast<uint8_t>(glm::clamp(h, 0.f, 255.f));
                size_t idx = (y * m_Params.width + x) * m_Params.channels;
                m_ImageData[idx + 0] = v;    // R
                m_ImageData[idx + 1] = v;    // G
                m_ImageData[idx + 2] = v;    // B
                m_ImageData[idx + 3] = 255;  // A
            }
        }
    }

    void HeightMapEditor::StartWorker() {
        m_NeedRegen = false;
        m_WorkerBusy = false;
        m_NewImageReady = false;
        m_WorkerThread = std::thread([this]() { this->WorkerLoop(); });
    }

    void HeightMapEditor::StopWorker() {
        m_NeedRegen = false;
        m_WorkerBusy = false;
        m_WorkerStop = true;
        if (m_WorkerThread.joinable()) m_WorkerThread.join();
    }

    void HeightMapEditor::WorkerLoop() {
        while (!m_WorkerStop) {
            if (!m_NeedRegen) {
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
                continue;
            }
            m_NeedRegen = false;
            m_WorkerBusy = true;

            auto params = m_Params;
            auto points = m_Curve.GetSortedPoints();
            std::vector<uint8_t> tmpData(params.width * params.height * params.channels);

            siv::PerlinNoise perlin(params.seed);
            for (uint32_t y = 0; y < params.height; ++y) {
                for (uint32_t x = 0; x < params.width; ++x) {
                    float nx = float(x) / float(params.width - 1);
                    float ny = float(y) / float(params.height - 1);
                    double n = perlin.octave2D(nx / params.scale, ny / params.scale, params.octaves);
                    n = glm::clamp(float(n), -1.f, 1.f);
                    float fn = static_cast<float>(n);
                    float curve_x = glm::clamp((fn + 1.f) * 0.5f, 0.0f, 1.0f);
                    float h = m_Curve.Evaluate(curve_x, points);
                    uint8_t v = static_cast<uint8_t>(glm::clamp(h, 0.f, 255.f));
                    size_t idx = (y * params.width + x) * params.channels;
                    tmpData[idx + 0] = v;
                    tmpData[idx + 1] = v;
                    tmpData[idx + 2] = v;
                    tmpData[idx + 3] = 255;
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_ImageMutex);
                m_WorkerImageData.swap(tmpData);
                m_NewImageReady = true;
            }
            m_WorkerBusy = false;
        }
    }
}
