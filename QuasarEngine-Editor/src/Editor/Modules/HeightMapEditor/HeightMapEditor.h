#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <random>
#include <fstream>
#include <algorithm>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Curve.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Tools/PerlinNoise.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <Editor/Modules/IEditorModule.h>
#include <Editor/EditorSplitters.h>
#include <Editor/UndoStack.h>

namespace QuasarEngine
{
    enum class PreviewMode : uint8_t
    {
        Grayscale = 0,
        Normals = 1
    };

    enum class FractalType : uint8_t
    {
        FBM = 0,
        Billow = 1,
        Ridged = 2
    };

    struct DomainWarpParams {
        bool  enabled = false;
        float amount = 0.15f;
        float frequency = 1.5f;
        int   octaves = 3;
    };

    struct TerracingParams {
        bool  enabled = false;
        int   steps = 8;
        float jitter = 0.05f;
    };

    struct TilingParams {
        bool  seamless = false;
        float periodX = 1.0f;
        float periodY = 1.0f;
    };

    struct HeightMapParams {
        uint32_t width = 512;
        uint32_t height = 512;
        uint32_t channels = 4;

        int   octaves = 6;
        float scaleX = 0.02f;
        float scaleY = 0.02f;
        float lacunarity = 2.0f;
        float persistence = 0.5f;
        FractalType fractal = FractalType::FBM;

        uint32_t seed = 123456789;

        DomainWarpParams warp;
        TerracingParams  terracing;
        TilingParams     tiling;

        PreviewMode previewMode = PreviewMode::Grayscale;
        float       waterLevel = -1.f;
    };

    struct GenerationSnapshot
    {
        HeightMapParams params;
        std::vector<glm::vec2> curvePointsSorted;
    };

    struct EditorState
    {
        HeightMapParams params;
        std::vector<glm::vec2> curvePoints;
    };

    class HeightMapEditor : public IEditorModule {
    public:
        HeightMapEditor(EditorContext& context);
        ~HeightMapEditor() override;

        void Update(double dt) override;
        void Render() override;
        void RenderUI() override;

        const std::vector<uint8_t>& GetImageData()  const { return m_ImageRGBA8; }
        const std::vector<uint16_t>& GetHeight16()   const { return m_Height16; }
        const std::vector<float>& GetHeightFloat()const { return m_HeightF; }
        const Curve& GetCurve() const { return m_Curve; }
        const HeightMapParams& GetParams() const { return m_Params; }

        bool SaveRAW16(const char* filePath) const;
        void NormalizeHeights();
        void ResetParams();

    private:
        void StartWorker();
        void StopWorker();
        void RequestRegen();
        void WorkerLoop();
        void BuildPreviewFromHeight(const GenerationSnapshot& snap);
        void MakeTexture();
        void MarkDirty();

        void DrawCurveEditor(ImDrawList* dl, const ImVec2& origin, const ImVec2& size);
        void DrawRightPanel(float width, float height);
        void DrawImagePreview();
        void DrawHistogram();

        static float EvalFractal2D(const siv::PerlinNoise& perlin, float x, float y, const HeightMapParams& p);
        static float EvalFractal2D_Tileable(const siv::PerlinNoise& perlin, float x, float y, const HeightMapParams& p);
        static float ApplyTerracing(float h, const TerracingParams& t, uint32_t seed);

        void PushUndo(const char* reason = nullptr);
        void DoUndo();
        void DoRedo();

        void ValidateParams();

        void BuildPreviewFromHeight(const HeightMapParams& p, const std::vector<glm::vec2>&);

        Curve m_Curve;

        HeightMapParams m_Params;

        std::vector<uint8_t>  m_ImageRGBA8;
        std::vector<float>    m_HeightF;
        std::vector<uint16_t> m_Height16;
        std::shared_ptr<Texture2D> m_Texture;

        std::array<uint32_t, 256> m_Hist = { 0 };
        uint32_t m_HistMax = 0;

        bool   m_AutoGenerate = true;
        bool   m_ShowGrid = true;
        bool   m_SnapX = false;
        bool   m_SnapY = false;
        float  m_SnapXStep = 0.05f;
        float  m_SnapYStep = 16.0f;
        float  m_Zoom = 1.0f;
        ImVec2 m_Pan = ImVec2(0, 0);
        float  m_RefreshInterval = 0.07f;
        double m_LastRegenTime = 0.0;
        bool   m_Dragging = false;

        UndoStack<EditorState> m_UndoStack{ 64 };

        std::thread m_Worker;
        std::mutex  m_StateMutex;
        std::mutex  m_ImageMutex;
        std::condition_variable m_CV;
        std::atomic<bool> m_WorkerStop{ false };
        std::atomic<bool> m_NeedRegen{ false };
        std::atomic<bool> m_CancelJob{ false };

        std::vector<float>    m_WorkerHeightF;
        std::vector<uint16_t> m_WorkerHeight16;
        std::vector<uint8_t>  m_WorkerPreviewRGBA8;
        GenerationSnapshot    m_LastSnapshot;

        std::atomic<bool> m_NewImageReady{ false };
        bool m_ImageDirty = false;

        double m_LastUpdateTime = 0.0;
    };
}
