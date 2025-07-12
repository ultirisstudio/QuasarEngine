#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <thread>

#include "Curve.h"

#include <imgui/imgui.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Tools/PerlinNoise.h>

namespace QuasarEngine
{
    struct HeightMapParams {
        uint32_t width = 512;
        uint32_t height = 512;
        uint32_t channels = 4;
        int octaves = 6;
        float scale = 0.02f;
        uint32_t seed = 123456789;
    };

    class HeightMapEditor {
    public:
        HeightMapEditor();
        ~HeightMapEditor();

        void OnImGuiRender(const char* windowName = "Height Map Editor");

        void Update();

        const std::vector<uint8_t>& GetImageData() const { return m_ImageData; }
        const Curve& GetCurve() const { return m_Curve; }
        const HeightMapParams& GetParams() const { return m_Params; }
    private:
        void GenerateImage();
        void DrawCurveEditor(ImDrawList* draw_list, const ImVec2& origin, const ImVec2& size);
        void DrawImagePreview();
        void RefreshTexture();

        void StartWorker();
        void StopWorker();
        void WorkerLoop();

        Curve m_Curve;
        HeightMapParams m_Params;
        std::vector<uint8_t> m_ImageData;
        std::shared_ptr<Texture2D> m_Texture;

        std::atomic<bool> m_NeedRegen;
        std::atomic<bool> m_WorkerBusy;
        std::atomic<bool> m_NewImageReady;
        std::atomic<bool> m_WorkerStop;
        std::thread m_WorkerThread;
        std::mutex m_ImageMutex;
        std::vector<uint8_t> m_WorkerImageData;

        bool m_ImageDirty;
        double m_LastUpdateTime;
        double m_LastDirtyTime;
        bool m_Dragging;
        float m_RefreshInterval;
    };
}
