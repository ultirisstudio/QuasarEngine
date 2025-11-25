#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace QuasarEngine
{
    struct EditorCanvas
    {
        ImVec2 pan{ 0.0f, 0.0f };
        float  zoom = 1.0f;

        ImVec2 canvasPos{ 0.0f, 0.0f };
        ImVec2 canvasSize{ 0.0f, 0.0f };

        bool  showGrid = true;
        float baseGridStep = 40.0f;

        void BeginRegion(ImVec2 pos, ImVec2 size)
        {
            canvasPos = pos;
            canvasSize = size;
        }

        ImVec2 ScreenToCanvas(ImVec2 screen) const
        {
            ImVec2 p = screen - canvasPos;
            return (p - pan) / zoom;
        }

        ImVec2 CanvasToScreen(ImVec2 canvas) const
        {
            return canvas * zoom + pan + canvasPos;
        }

        void DrawGrid(ImDrawList* dl) const
        {
            if (!showGrid)
                return;

            float step = baseGridStep * zoom;
            if (step <= 0.0f)
                return;

            ImU32 minor = IM_COL32(60, 62, 68, 120);
            ImVec2 origin = canvasPos;

            float startX = std::fmod(pan.x, step);
            float startY = std::fmod(pan.y, step);

            for (float x = startX; x < canvasSize.x; x += step)
                dl->AddLine(origin + ImVec2(x, 0.0f), origin + ImVec2(x, canvasSize.y), minor);
            for (float y = startY; y < canvasSize.y; y += step)
                dl->AddLine(origin + ImVec2(0.0f, y), origin + ImVec2(canvasSize.x, y), minor);
        }

        void HandlePan(const ImGuiIO& io, bool hovered)
        {
            static bool  panning = false;
            static ImVec2 panStart{ 0.0f, 0.0f };

            if (!hovered)
            {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                    panning = false;
                return;
            }

            if (!panning && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                panning = true;
                panStart = io.MousePos;
            }
            else if (panning && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                ImVec2 delta = io.MousePos - panStart;
                pan.x += delta.x;
                pan.y += delta.y;
                panStart = io.MousePos;
            }
            else if (panning && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                panning = false;
            }
        }

        void HandleZoom(const ImGuiIO& io, bool hovered,
            float minZoom = 0.25f, float maxZoom = 4.0f)
        {
            if (!hovered)
                return;

            float wheel = io.MouseWheel;
            if (wheel == 0.0f)
                return;

            float prevZoom = zoom;
            zoom = std::clamp(zoom + wheel * 0.1f, minZoom, maxZoom);

            ImVec2 mouse = io.MousePos;
            pan = (pan - mouse) * (zoom / prevZoom) + mouse;
        }

        void HandlePanAndZoom(const ImGuiIO& io, bool hovered,
            float minZoom = 0.25f, float maxZoom = 4.0f)
        {
            HandlePan(io, hovered);
            HandleZoom(io, hovered, minZoom, maxZoom);
        }
    };
}