#include "HeightMapEditor.h"

namespace QuasarEngine
{
    static inline float saturate(float x) { return glm::clamp(x, 0.0f, 1.0f); }

    static inline uint16_t f2u16(float v) {
        v = saturate(v);
        return static_cast<uint16_t>(std::round(v * 65535.0f));
    }

    static inline uint8_t f2u8(float v) {
        v = saturate(v);
        return static_cast<uint8_t>(std::round(v * 255.0f));
    }

    float HeightMapEditor::ApplyTerracing(float h, const TerracingParams& t, uint32_t seed)
    {
        if (!t.enabled || t.steps <= 1) return saturate(h);
        float step = 1.0f / float(t.steps);
        float q = std::floor(h / step) * step;

        uint32_t s = seed * 1664525u + 1013904223u;
        float r = ((s & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
        float j = t.jitter * r * step;

        return saturate(q + j);
    }

    float HeightMapEditor::EvalFractal2D(const siv::PerlinNoise& perlin, float x, float y,
        const HeightMapParams& p)
    {
        float amp = 1.0f;
        float freqX = 1.0f;
        float freqY = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < p.octaves; ++i)
        {
            float nx = x * freqX;
            float ny = y * freqY;
            double n = perlin.noise2D(nx, ny);
            float f = static_cast<float>(n);

            switch (p.fractal)
            {
            case FractalType::FBM:    f = f; break;
            case FractalType::Billow: f = std::abs(f); break;
            case FractalType::Ridged: f = 1.0f - std::abs(f); break;
            }

            sum += f * amp;
            norm += amp;

            amp *= p.persistence;
            freqX *= p.lacunarity;
            freqY *= p.lacunarity;
        }

        if (norm <= 1e-6f) return 0.0f;
        return glm::clamp(sum / norm, -1.0f, 1.0f);
    }

    float HeightMapEditor::EvalFractal2D_Tileable(const siv::PerlinNoise& perlin, float x, float y, const HeightMapParams& p)
    {
        float Px = glm::max(p.tiling.periodX, 1e-4f);
        float Py = glm::max(p.tiling.periodY, 1e-4f);

        float tx = x / Px - std::floor(x / Px);
        float ty = y / Py - std::floor(y / Py);

        auto eval = [&](float sx, float sy) -> float {
            HeightMapParams tmp = p;
            tmp.tiling.seamless = false;
            return EvalFractal2D(perlin, sx, sy, tmp);
            };

        float a = eval(x, y);
        float b = eval(x + Px, y);
        float c = eval(x, y + Py);
        float d = eval(x + Px, y + Py);

        float ab = a * (1.0f - tx) + b * tx;
        float cd = c * (1.0f - tx) + d * tx;
        float v = ab * (1.0f - ty) + cd * ty;

        return glm::clamp(v, -1.0f, 1.0f);
    }

    HeightMapEditor::HeightMapEditor()
    {
        m_Curve.AddPoint({ 0.05f, 200.f });
        m_Curve.AddPoint({ 0.20f, 140.f });
        m_Curve.AddPoint({ 0.40f, 110.f });
        m_Curve.AddPoint({ 0.65f, 120.f });
        m_Curve.AddPoint({ 0.95f, 210.f });

        m_ImageRGBA8.resize(m_Params.width * m_Params.height * m_Params.channels, 0);
        m_HeightF.resize(m_Params.width * m_Params.height, 0.0f);
        m_Height16.resize(m_Params.width * m_Params.height, 0);

        MakeTexture();

        StartWorker();
        RequestRegen();
    }

    HeightMapEditor::~HeightMapEditor()
    {
        StopWorker();
        m_Texture.reset();
    }

    void HeightMapEditor::StartWorker()
    {
        m_WorkerStop = false;
        m_CancelJob = false;
        m_Worker = std::thread([this]() { WorkerLoop(); });
    }

    void HeightMapEditor::StopWorker()
    {
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            m_WorkerStop = true;
            m_CancelJob = true;
        }
        m_CV.notify_all();
        if (m_Worker.joinable()) m_Worker.join();
    }

    void HeightMapEditor::RequestRegen()
    {
        m_NeedRegen = true;
        m_CancelJob = true;
        m_CV.notify_all();
    }

    void HeightMapEditor::WorkerLoop()
    {
        for (;;)
        {
            {
                std::unique_lock<std::mutex> lk(m_StateMutex);
                m_CV.wait(lk, [&]() { return m_WorkerStop || m_NeedRegen; });
                if (m_WorkerStop) break;

                m_NeedRegen = false;
                m_CancelJob = false;

                m_LastSnapshot.params = m_Params;
                m_LastSnapshot.curvePointsSorted = m_Curve.GetSortedPoints();
            }

            const auto& snap = m_LastSnapshot;
            const uint32_t W = glm::clamp<uint32_t>(snap.params.width, 1u, 8192u);
            const uint32_t H = glm::clamp<uint32_t>(snap.params.height, 1u, 8192u);

            std::vector<float>    outH;
            std::vector<uint16_t> outH16;
            std::vector<uint8_t>  outRGBA;
            outH.resize(W * H);
            outH16.resize(W * H);
            outRGBA.resize(size_t(W) * H * snap.params.channels);

            siv::PerlinNoise perlin(snap.params.seed);

            auto evalCurve = [&](float n01)->float {
                float h255 = m_Curve.Evaluate(glm::clamp(n01, 0.0f, 1.0f), snap.curvePointsSorted);
                return glm::clamp(h255 / 255.0f, 0.0f, 1.0f);
                };

            for (uint32_t y = 0; y < H; ++y)
            {
                if (m_CancelJob) break;

                for (uint32_t x = 0; x < W; ++x)
                {
                    float nx = float(x) / float(W - 1);
                    float ny = float(y) / float(H - 1);

                    float sx = nx / glm::max(snap.params.scaleX, 1e-6f);
                    float sy = ny / glm::max(snap.params.scaleY, 1e-6f);

                    if (snap.params.warp.enabled) {
                        float wx = EvalFractal2D(perlin, sx * snap.params.warp.frequency,
                            sy * snap.params.warp.frequency, snap.params);
                        float wy = EvalFractal2D(perlin, (sx + 37.1f) * snap.params.warp.frequency,
                            (sy - 11.3f) * snap.params.warp.frequency, snap.params);
                        sx += wx * snap.params.warp.amount;
                        sy += wy * snap.params.warp.amount;
                    }

                    float n = 0.0f;
                    if (snap.params.tiling.seamless)
                        n = EvalFractal2D_Tileable(perlin, sx, sy, snap.params);
                    else
                        n = EvalFractal2D(perlin, sx, sy, snap.params);

                    float n01 = (n + 1.0f) * 0.5f;

                    float h = evalCurve(n01);

                    h = ApplyTerracing(h, snap.params.terracing, (x * 73856093u) ^ (y * 19349663u) ^ snap.params.seed);

                    size_t idx = size_t(y) * W + x;
                    outH[idx] = h;
                }
            }

            if (m_CancelJob) continue;

            // Construire H16
            for (size_t i = 0; i < outH.size(); ++i)
                outH16[i] = f2u16(outH[i]);

            for (size_t i = 0; i < outH.size(); ++i)
            {
                uint8_t v = f2u8(outH[i]);
                size_t idx = i * snap.params.channels;
                outRGBA[idx + 0] = v;
                outRGBA[idx + 1] = v;
                outRGBA[idx + 2] = v;
                outRGBA[idx + 3] = 255;
            }

            {
                std::lock_guard<std::mutex> lk(m_ImageMutex);
                m_WorkerHeightF.swap(outH);
                m_WorkerHeight16.swap(outH16);
                m_WorkerPreviewRGBA8.swap(outRGBA);
                m_NewImageReady = true;
            }
        }
    }

    void HeightMapEditor::BuildPreviewFromHeight(const GenerationSnapshot& snap)
    {
        const uint32_t W = snap.params.width;
        const uint32_t H = snap.params.height;

        if (m_ImageRGBA8.size() != size_t(W) * H * snap.params.channels)
            m_ImageRGBA8.resize(size_t(W) * H * snap.params.channels);

        if (snap.params.previewMode == PreviewMode::Grayscale)
        {
            for (size_t i = 0; i < m_HeightF.size(); ++i) {
                uint8_t v = f2u8(m_HeightF[i]);
                size_t idx = i * snap.params.channels;
                m_ImageRGBA8[idx + 0] = v;
                m_ImageRGBA8[idx + 1] = v;
                m_ImageRGBA8[idx + 2] = v;
                m_ImageRGBA8[idx + 3] = 255;
            }
        }
        else
        {
            auto Hf = [&](int x, int y)->float {
                x = glm::clamp(x, 0, int(W) - 1);
                y = glm::clamp(y, 0, int(H) - 1);
                return m_HeightF[size_t(y) * W + x];
                };

            for (uint32_t y = 0; y < H; ++y)
                for (uint32_t x = 0; x < W; ++x)
                {
                    float hL = Hf(int(x) - 1, y);
                    float hR = Hf(int(x) + 1, y);
                    float hD = Hf(x, int(y) - 1);
                    float hU = Hf(x, int(y) + 1);

                    glm::vec3 n;
                    n.x = (hL - hR);
                    n.y = (hD - hU);
                    n.z = 2.0f / glm::max(snap.params.scaleX + snap.params.scaleY, 1e-3f);
                    n = glm::normalize(n);

                    uint8_t r = f2u8(n.x * 0.5f + 0.5f);
                    uint8_t g = f2u8(1.0f - (n.y * 0.5f + 0.5f));
                    uint8_t b = f2u8(n.z * 0.5f + 0.5f);

                    size_t idx = (size_t(y) * W + x) * snap.params.channels;
                    m_ImageRGBA8[idx + 0] = r;
                    m_ImageRGBA8[idx + 1] = g;
                    m_ImageRGBA8[idx + 2] = b;
                    m_ImageRGBA8[idx + 3] = 255;
                }
        }

        if (snap.params.waterLevel >= 0.0f)
        {
            const float wl = saturate(snap.params.waterLevel);
            for (size_t i = 0; i < m_HeightF.size(); ++i)
            {
                if (m_HeightF[i] < wl)
                {
                    size_t idx = i * snap.params.channels;
                    
                    m_ImageRGBA8[idx + 0] = uint8_t(float(m_ImageRGBA8[idx + 0]) * 0.5f);
                    m_ImageRGBA8[idx + 1] = uint8_t(float(m_ImageRGBA8[idx + 1]) * 0.6f);
                    m_ImageRGBA8[idx + 2] = uint8_t(glm::min(255.0f, float(m_ImageRGBA8[idx + 2]) * 0.5f + 128.0f));
                }
            }
        }

        m_Hist.fill(0);
        for (float h : m_HeightF) {
            uint32_t bin = f2u8(h);
            m_Hist[bin]++;
        }
        m_HistMax = 0;
        for (uint32_t v : m_Hist) m_HistMax = std::max(m_HistMax, v);
    }

    void HeightMapEditor::MakeTexture()
    {
        TextureSpecification spec;
        spec.width = m_Params.width;
        spec.height = m_Params.height;
        spec.channels = m_Params.channels;
        spec.compressed = false;

        if (!AssetManager::Instance().isAssetLoaded("heightmap_preview"))
        {
            AssetToLoad asset;
            asset.type = AssetType::TEXTURE;
            asset.id = "heightmap_preview";
            asset.data = m_ImageRGBA8.data();
            asset.size = m_ImageRGBA8.size();
            asset.spec = spec;
            AssetManager::Instance().loadAsset(asset);
        }
        else
        {
            AssetToLoad asset;
            asset.type = AssetType::TEXTURE;
            asset.id = "heightmap_preview";
            asset.data = m_ImageRGBA8.data();
            asset.size = m_ImageRGBA8.size();
            asset.spec = spec;
            AssetManager::Instance().updateAsset(asset);
        }

        m_Texture = AssetManager::Instance().getAsset<Texture2D>("heightmap_preview");
    }

    void HeightMapEditor::MarkDirty()
    {
        m_ImageDirty = true;
    }

    void HeightMapEditor::OnImGuiRender(const char* windowName)
    {
        ImGui::SetNextWindowSize(ImVec2(1100, 680), ImGuiCond_Once);
        ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float left_ratio = 0.55f;
        float min_left = 280.f;
        float left_w = ImMax(avail.x * left_ratio, min_left);

        ImGui::BeginChild("##CurveEditorPanel", ImVec2(left_w, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImVec2 child = ImGui::GetContentRegionAvail();
            ImVec2 size = ImVec2(ImMax(child.x - 10.f, 360.f), ImMax(child.y - 10.f, 300.f));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 origin = ImGui::GetCursorScreenPos();

            DrawCurveEditor(dl, origin, size);
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##splitter_v", ImVec2(8, avail.y));
        if (ImGui::IsItemActive()) {
            left_ratio += ImGui::GetIO().MouseDelta.x / avail.x;
            left_ratio = glm::clamp(left_ratio, 0.25f, 0.80f);
        }
        ImGui::SameLine(0, 0);

        float right_w = avail.x - left_w - 8.0f;
        ImGui::BeginChild("##RightPanel", ImVec2(right_w, 0), true, ImGuiWindowFlags_NoScrollWithMouse);
        {
            DrawRightPanel(right_w, ImGui::GetContentRegionAvail().y);
        }
        ImGui::EndChild();

        if (m_NewImageReady)
        {
            {
                std::lock_guard<std::mutex> lk(m_ImageMutex);
                m_HeightF.swap(m_WorkerHeightF);
                m_Height16.swap(m_WorkerHeight16);
                m_ImageRGBA8.swap(m_WorkerPreviewRGBA8);
                m_NewImageReady = false;
            }
            {
                std::lock_guard<std::mutex> lk2(m_StateMutex);
                BuildPreviewFromHeight(m_LastSnapshot);
            }
            MakeTexture();
            m_ImageDirty = false;
        }

        if (m_ImageDirty) {
            GenerationSnapshot snap;
            {
                std::lock_guard<std::mutex> lk(m_StateMutex);
                snap.params = m_Params;
                snap.curvePointsSorted = m_Curve.GetSortedPoints();
            }
            BuildPreviewFromHeight(snap);
            MakeTexture();
            m_ImageDirty = false;
        }


        ImGui::End();
    }

    void HeightMapEditor::Update()
    {
        
    }

    void HeightMapEditor::DrawCurveEditor(ImDrawList* dl, const ImVec2& origin, const ImVec2& size)
    {
        ImGuiIO& io = ImGui::GetIO();
        double now = io.DeltaTime > 0.0 ? ImGui::GetTime() : m_LastUpdateTime;

        ImU32 bg = IM_COL32(20, 22, 25, 255);
        ImU32 border = IM_COL32(180, 180, 180, 255);
        dl->AddRectFilled(origin, origin + size, bg, 6.0f);
        dl->AddRect(origin, origin + size, border, 6.0f);

        if (m_ShowGrid)
        {
            const int gridX = 10;
            const int gridY = 8;
            for (int i = 1; i < gridX; ++i) {
                float t = float(i) / gridX;
                float x = origin.x + t * size.x;
                dl->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), IM_COL32(60, 60, 60, 255));
            }
            for (int j = 1; j < gridY; ++j) {
                float t = float(j) / gridY;
                float y = origin.y + t * size.y;
                dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + size.x, y), IM_COL32(60, 60, 60, 255));
            }
        }

        std::vector<glm::vec2> pts;
        std::vector<glm::vec2> sorted;
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            pts = m_Curve.GetPoints();
            sorted = m_Curve.GetSortedPoints();
        }

        std::vector<glm::vec2> interp = m_Curve.InterpolateCurve(sorted, 256);
        for (size_t i = 1; i < interp.size(); ++i) {
            ImVec2 p0 = { origin.x + interp[i - 1].x * size.x,
                          origin.y + (1.0f - interp[i - 1].y / 255.f) * size.y };
            ImVec2 p1 = { origin.x + interp[i].x * size.x,
                          origin.y + (1.0f - interp[i].y / 255.f) * size.y };
            dl->AddLine(p0, p1, IM_COL32(40, 200, 90, 255), 2.0f);
        }

        bool any_drag = false;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            auto pt = pts[i];
            ImVec2 p = { origin.x + pt.x * size.x, origin.y + (1.0f - pt.y / 255.f) * size.y };
            float r = 6.0f;
            dl->AddCircleFilled(p, r, IM_COL32(250, 70, 70, 255));
            ImGui::SetCursorScreenPos({ p.x - r, p.y - r });
            ImGui::InvisibleButton(("pt" + std::to_string(i)).c_str(), ImVec2(r * 2, r * 2));

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
            {
                ImVec2 d = io.MouseDelta;
                float nx = (p.x + d.x - origin.x) / size.x;
                float ny = 1.0f - (p.y + d.y - origin.y) / size.y;

                if (m_SnapX) nx = std::round(nx / m_SnapXStep) * m_SnapXStep;
                if (m_SnapY) ny = std::round((ny * 255.f) / m_SnapYStep) * (m_SnapYStep / 255.f);

                nx = glm::clamp(nx, 0.f, 1.f);
                ny = glm::clamp(ny, 0.f, 1.f);

                {
                    std::lock_guard<std::mutex> lk(m_StateMutex);
                    m_Curve.MovePoint(i, { nx, ny * 255.f });
                }

                any_drag = true;

                if (m_AutoGenerate && (now - m_LastRegenTime > m_RefreshInterval)) {
                    RequestRegen();
                    m_LastRegenTime = now;
                }
            }
            else if (ImGui::IsItemDeactivated())
            {
                if (m_AutoGenerate) RequestRegen();
                PushUndo("Move curve point");
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && pts.size() > 2) {
                {
                    std::lock_guard<std::mutex> lk(m_StateMutex);
                    m_Curve.RemovePoint(i);
                }
                if (m_AutoGenerate) RequestRegen();
                PushUndo("Remove curve point");
                break;
            }
        }
        m_Dragging = any_drag;

        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::Button("Ajouter point")) {
            {
                std::lock_guard<std::mutex> lk(m_StateMutex);
                m_Curve.AddPoint({ 0.5f, 128.f });
            }
            if (m_AutoGenerate) RequestRegen();
            PushUndo("Add curve point");
        }
        ImGui::SameLine();
        if (ImGui::Button("Preset: Linéaire")) {
            {
                std::lock_guard<std::mutex> lk(m_StateMutex);
                m_Curve.Clear();
                m_Curve.AddPoint({ 0.f, 0.f });
                m_Curve.AddPoint({ 1.f, 255.f });
            }
            if (m_AutoGenerate) RequestRegen();
            PushUndo("Preset Linear");
        }
        ImGui::SameLine();
        if (ImGui::Button("Preset: S-Curve")) {
            {
                std::lock_guard<std::mutex> lk(m_StateMutex);
                m_Curve.Clear();
                m_Curve.AddPoint({ 0.f, 0.f });
                m_Curve.AddPoint({ 0.25f, 64.f });
                m_Curve.AddPoint({ 0.75f, 192.f });
                m_Curve.AddPoint({ 1.f, 255.f });
            }
            if (m_AutoGenerate) RequestRegen();
            PushUndo("Preset S");
        }

        ImGui::Separator();
        ImGui::Checkbox("Grille", &m_ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Snap X", &m_SnapX);
        if (m_SnapX) { ImGui::SameLine(); ImGui::DragFloat("Pas X", &m_SnapXStep, 0.005f, 0.01f, 0.25f, "%.3f"); }
        ImGui::SameLine();
        ImGui::Checkbox("Snap Y", &m_SnapY);
        if (m_SnapY) { ImGui::SameLine(); ImGui::DragFloat("Pas Y", &m_SnapYStep, 1.0f, 1.0f, 64.0f, "%.0f"); }
    }

    void HeightMapEditor::DrawRightPanel(float width, float height)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        static float preview_ratio = 0.60f;
        float min_h = 180.f;
        float preview_h = ImMax(avail.y * preview_ratio, min_h);

        ImGui::BeginChild("##PreviewPanel", ImVec2(-1, preview_h), true);
        {
            ImGui::Text("Prévisualisation");
            ImGui::Separator();
            DrawImagePreview();
            ImGui::Dummy(ImVec2(0, 6));
            DrawHistogram();
        }
        ImGui::EndChild();

        ImGui::InvisibleButton("##splitter_h", ImVec2(-1, 8));
        if (ImGui::IsItemActive()) {
            preview_ratio += ImGui::GetIO().MouseDelta.y / avail.y;
            preview_ratio = glm::clamp(preview_ratio, 0.25f, 0.85f);
        }

        ImGui::BeginChild("##ParamsPanel", ImVec2(-1, -1), true);
        {
            bool regen = false;
            bool previewDirty = false;
            bool pushedUndo = false;

            ImGui::Text("Paramètres");
            ImGui::Separator();

            HeightMapParams ui;
            {
                std::lock_guard<std::mutex> lk(m_StateMutex);
                ui = m_Params;
            }
            const HeightMapParams before = ui;

            ImGui::Checkbox("Auto-générer", &m_AutoGenerate);
            ImGui::SameLine();
            if (ImGui::Button("Générer maintenant")) regen = true;

            ImGui::SameLine();
            if (ImGui::Button("Seed aléatoire")) {
                std::random_device rd; std::mt19937 rng(rd());
                std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFF0u);
                ui.seed = dist(rng);
                regen = true;
                pushedUndo = true;
            }

            ImGui::DragFloat("Debounce (s)", &m_RefreshInterval, 0.005f, 0.01f, 0.5f, "%.3f");

            uint32_t w = ui.width, h = ui.height;
            if (ImGui::InputScalar("Largeur", ImGuiDataType_U32, &w)) { ui.width = glm::clamp<uint32_t>(w, 1u, 8192u); regen = true; }
            if (ImGui::InputScalar("Hauteur", ImGuiDataType_U32, &h)) { ui.height = glm::clamp<uint32_t>(h, 1u, 8192u); regen = true; }

            int oct = ui.octaves;
            if (ImGui::SliderInt("Octaves", &oct, 1, 12)) { ui.octaves = oct; regen = true; }

            if (ImGui::DragFloat("Scale X", &ui.scaleX, 0.0005f, 0.001f, 0.2f, "%.4f")) regen = true;
            if (ImGui::DragFloat("Scale Y", &ui.scaleY, 0.0005f, 0.001f, 0.2f, "%.4f")) regen = true;

            if (ImGui::DragFloat("Lacunarity", &ui.lacunarity, 0.01f, 1.1f, 4.0f)) regen = true;
            if (ImGui::DragFloat("Persistence", &ui.persistence, 0.01f, 0.1f, 0.9f)) regen = true;

            int ft = int(ui.fractal);
            if (ImGui::Combo("Fractal", &ft, "FBM\0Billow\0Ridged\0\0")) { ui.fractal = (FractalType)ft; regen = true; }

            if (ImGui::CollapsingHeader("Domain Warp", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Activer warp", &ui.warp.enabled)) regen = true;
                if (ui.warp.enabled) {
                    if (ImGui::DragFloat("Warp amount", &ui.warp.amount, 0.005f, 0.0f, 1.0f)) regen = true;
                    if (ImGui::DragFloat("Warp frequency", &ui.warp.frequency, 0.01f, 0.1f, 8.0f)) regen = true;
                    if (ImGui::SliderInt("Warp octaves", &ui.warp.octaves, 1, 8)) regen = true;
                }
            }

            if (ImGui::CollapsingHeader("Terracing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Activer terracing", &ui.terracing.enabled)) regen = true;
                if (ui.terracing.enabled) {
                    if (ImGui::SliderInt("Steps", &ui.terracing.steps, 2, 64)) regen = true;
                    if (ImGui::DragFloat("Jitter", &ui.terracing.jitter, 0.005f, 0.0f, 0.25f)) regen = true;
                }
            }

            if (ImGui::CollapsingHeader("Carrelage (Seamless)"))
            {
                if (ImGui::Checkbox("Seamless", &ui.tiling.seamless)) regen = true;
                if (ui.tiling.seamless) {
                    if (ImGui::DragFloat("Période X", &ui.tiling.periodX, 0.01f, 0.1f, 64.0f)) regen = true;
                    if (ImGui::DragFloat("Période Y", &ui.tiling.periodY, 0.01f, 0.1f, 64.0f)) regen = true;
                }
            }

            int pm = int(ui.previewMode);
            if (ImGui::Combo("Mode Preview", &pm, "Grayscale\0Normals\0\0")) {
                ui.previewMode = (PreviewMode)pm;
                previewDirty = true;
            }

            bool waterEnabled = (ui.waterLevel >= 0.0f);
            if (ImGui::Checkbox("Water level actif", &waterEnabled)) {
                if (waterEnabled) {
                    if (ui.waterLevel < 0.0f) ui.waterLevel = 0.35f;
                }
                else {
                    ui.waterLevel = -1.0f;
                }
                previewDirty = true;
            }
            if (waterEnabled) {
                if (ImGui::SliderFloat("Water level", &ui.waterLevel, 0.0f, 1.0f))
                    previewDirty = true;
            }

            ImGui::Separator();
            if (ImGui::Button("Normaliser (min/max)")) { NormalizeHeights(); }
            ImGui::SameLine();
            if (ImGui::Button("Reset paramètres")) {
                HeightMapParams def;
                def.previewMode = ui.previewMode;
                ui = def;
                regen = true;
                pushedUndo = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Undo")) DoUndo();
            ImGui::SameLine();
            if (ImGui::Button("Redo")) DoRedo();

            if (ImGui::Button("Exporter RAW16 (height)")) {
                SaveRAW16("heightmap.raw16");
            }

            if (regen || previewDirty || std::memcmp(&before, &ui, sizeof(HeightMapParams)) != 0)
            {
                {
                    std::lock_guard<std::mutex> lk(m_StateMutex);
                    m_Params = ui;
                    ValidateParams();
                }
                if (regen)        RequestRegen();
                if (previewDirty) MarkDirty();
                if (!pushedUndo)  PushUndo("Edit params");
            }
        }
        ImGui::EndChild();
    }

    void HeightMapEditor::DrawImagePreview()
    {
        if (!AssetManager::Instance().isAssetLoaded("heightmap_preview"))
            return;

        if (!m_Texture) m_Texture = AssetManager::Instance().getAsset<Texture2D>("heightmap_preview");
        if (!m_Texture) return;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float scale = 0.5f;
        ImVec2 size = ImVec2(m_Params.width * scale, m_Params.height * scale);
        size.x = std::min(size.x, avail.x);
        size.y = std::min(size.y, avail.y - 80.f);
        ImGui::Image((ImTextureID)m_Texture->GetHandle(), size);
    }

    void HeightMapEditor::DrawHistogram()
    {
        ImGui::Separator();
        ImGui::Text("Histogramme (heights)");
        if (m_HistMax == 0) return;

        static std::vector<float> histF(256);
        for (int i = 0; i < 256; ++i) histF[i] = float(m_Hist[i]);

        ImGui::PlotHistogram("##h", histF.data(), 256, 0, nullptr, 0.0f, float(m_HistMax), ImVec2(-1, 80));
        ImGui::Text("Min: %u  Max: %u  Total: %u",
            (unsigned)std::distance(m_Hist.begin(), std::find_if(m_Hist.begin(), m_Hist.end(), [](uint32_t v) {return v > 0; })),
            (unsigned)(255 - std::distance(m_Hist.rbegin(), std::find_if(m_Hist.rbegin(), m_Hist.rend(), [](uint32_t v) {return v > 0; }))),
            (unsigned)std::accumulate(m_Hist.begin(), m_Hist.end(), 0u));
    }

    void HeightMapEditor::NormalizeHeights()
    {
        if (m_HeightF.empty()) return;

        float mn = std::numeric_limits<float>::infinity();
        float mx = -std::numeric_limits<float>::infinity();
        for (float v : m_HeightF) { mn = std::min(mn, v); mx = std::max(mx, v); }
        const float inv = (mx > mn) ? 1.0f / (mx - mn) : 1.0f;
        for (float& v : m_HeightF) v = (v - mn) * inv;

        if (m_Height16.size() != m_HeightF.size()) m_Height16.resize(m_HeightF.size());
        for (size_t i = 0; i < m_HeightF.size(); ++i)
            m_Height16[i] = f2u16(m_HeightF[i]);

        GenerationSnapshot snap;
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            snap.params = m_Params;
        }

        BuildPreviewFromHeight(snap);
        MakeTexture();
    }

    void HeightMapEditor::ResetParams()
    {
        std::lock_guard<std::mutex> lk(m_StateMutex);
        HeightMapParams def;
        def.previewMode = m_Params.previewMode;
        m_Params = def;
        PushUndo("Reset params");
    }

    bool HeightMapEditor::SaveRAW16(const char* filePath) const
    {
        if (m_Height16.empty()) return false;
        std::ofstream f(filePath, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(m_Height16.data()),
            std::streamsize(m_Height16.size() * sizeof(uint16_t)));
        return bool(f);
    }

    void HeightMapEditor::PushUndo(const char* /*reason*/)
    {
        EditorState s;
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            s.params = m_Params;
            s.curvePoints = m_Curve.GetPoints();
        }
        m_Undo.push_back(std::move(s));
        m_Redo.clear();
    }

    void HeightMapEditor::DoUndo()
    {
        if (m_Undo.empty()) return;
        EditorState cur;
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            cur.params = m_Params;
            cur.curvePoints = m_Curve.GetPoints();

            EditorState prev = m_Undo.back(); m_Undo.pop_back();
            m_Redo.push_back(cur);

            m_Params = prev.params;
            m_Curve.SetPoints(prev.curvePoints);
        }
        RequestRegen();
    }

    void HeightMapEditor::DoRedo()
    {
        if (m_Redo.empty()) return;
        EditorState next = m_Redo.back(); m_Redo.pop_back();
        EditorState cur;
        {
            std::lock_guard<std::mutex> lk(m_StateMutex);
            cur.params = m_Params;
            cur.curvePoints = m_Curve.GetPoints();

            m_Undo.push_back(cur);
            m_Params = next.params;
            m_Curve.SetPoints(next.curvePoints);
        }
        RequestRegen();
    }

    void HeightMapEditor::ValidateParams()
    {
        std::lock_guard<std::mutex> lk(m_StateMutex);
        m_Params.width = glm::clamp<uint32_t>(m_Params.width, 1u, 8192u);
        m_Params.height = glm::clamp<uint32_t>(m_Params.height, 1u, 8192u);
        m_Params.channels = 4;
        m_Params.scaleX = glm::clamp(m_Params.scaleX, 0.0005f, 1.0f);
        m_Params.scaleY = glm::clamp(m_Params.scaleY, 0.0005f, 1.0f);
        m_Params.lacunarity = glm::clamp(m_Params.lacunarity, 1.05f, 6.0f);
        m_Params.persistence = glm::clamp(m_Params.persistence, 0.1f, 0.95f);
        if (m_Params.tiling.seamless) {
            m_Params.tiling.periodX = glm::clamp(m_Params.tiling.periodX, 0.1f, 256.0f);
            m_Params.tiling.periodY = glm::clamp(m_Params.tiling.periodY, 0.1f, 256.0f);
        }
    }

    inline void BuildFrom(const HeightMapParams& params, const std::vector<glm::vec2>& sorted,
        std::vector<float>& heightF, std::vector<uint8_t>& rgba8,
        std::array<uint32_t, 256>& hist, uint32_t& histMax)
    {
        (void)params; (void)sorted; (void)heightF; (void)rgba8; (void)hist; (void)histMax;
    }

    void HeightMapEditor::BuildPreviewFromHeight(const HeightMapParams& p, const std::vector<glm::vec2>& /*sorted*/)
    {
        GenerationSnapshot snap; snap.params = p;
        BuildPreviewFromHeight(snap);
    }
}
