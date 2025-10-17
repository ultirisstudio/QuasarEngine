#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <optional>
#include <cstdint>
#include <algorithm>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/2D/SpriteComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
    class SpriteEditor
    {
    public:
        SpriteEditor();
        ~SpriteEditor() = default;

        void OnImGuiRender(const char* windowName = "Sprite Editor");

        void SetGrid(int gridW, int gridH) { m_GridW = std::max(1, gridW); m_GridH = std::max(1, gridH); ResizeLayers(); }
        void SetCellSize(float cellW, float cellH) { m_CellW = std::max(1.f, cellW); m_CellH = std::max(1.f, cellH); }
        void SetOrigin(const glm::vec2& origin) { m_Origin = origin; }

        struct Cell {
            UUID        entity = UUID::Null();
            std::string textureId;
            glm::vec4   uv{ 0,0,1,1 };
            bool        flipH = false;
            bool        flipV = false;
            glm::vec4   tint{ 1,1,1,1 };
        };
        struct Layer {
            std::string name = "Layer";
            int         sortingOrder = 0;
            bool        visible = true;
            std::vector<Cell> cells;
        };

        int  AddLayer(const std::string& name, int sortingOrder);
        void RemoveLayer(int index);
        void MoveLayer(int index, int delta);
        int  GetLayerCount() const { return (int)m_Layers.size(); }
        int  GetSelectedLayer() const { return m_SelectedLayer; }
        void SetSelectedLayer(int idx) { m_SelectedLayer = std::clamp(idx, 0, (int)m_Layers.size() - 1); }

    private:
        void DrawLeftPanel(float width, float height);
        void DrawCanvasPanel(float width, float height);
        void DrawRightPanel(float width, float height);
        void DrawTexturePalette();
        void DrawPlacedSpritesOnCanvas(ImDrawList* dl, const Layer& layer);

        void DrawPreviewPanel(float width, float height);
        void DrawGrid(ImDrawList* dl, ImVec2 origin, ImVec2 size, float step) const;

        struct Brush {
            std::string textureId;
            glm::vec4   uv{ 0,0,1,1 };
            bool        flipH = false;
            bool        flipV = false;
            glm::vec4   tint{ 1,1,1,1 };
        };

        void PaintAt(Scene& scene, Layer& layer, int cx, int cy, const Brush& b);
        void EraseAt(Scene& scene, Layer& layer, int cx, int cy);
        size_t CellIndex(int x, int y) const { return (size_t)y * (size_t)m_GridW + (size_t)x; }
        void ResizeLayers();

        static std::string ToProjectId(const std::filesystem::path& pAbsOrRel);
        ImTextureID GetImGuiTex(const std::string& projectId);
        static ImVec2 UV_GL_to_ImGui(const ImVec2& uv) { return ImVec2(uv.x, 1.0f - uv.y); }
        static void GLRectToImGuiUV(float u0, float v0, float u1, float v1, ImVec2& uv0, ImVec2& uv1) {
            uv0 = ImVec2(u0, 1.0f - v1);
            uv1 = ImVec2(u1, 1.0f - v0);
        }

    private:
        int   m_GridW = 32;
        int   m_GridH = 18;
        float m_CellW = 64.f;
        float m_CellH = 64.f;
        glm::vec2 m_Origin{ 0.f, 0.f };

        std::vector<Layer> m_Layers;
        int m_SelectedLayer = 0;

        std::vector<std::string> m_Palette;
        std::unordered_map<std::string, ImTextureID> m_TexCache;
        Brush m_Brush;

        ImVec2 m_CanvasPos{ 0,0 };
        ImVec2 m_CanvasSize{ 0,0 };
        ImVec2 m_Pan{ 20,20 };
        float  m_Zoom = 1.0f;
        bool   m_ShowGrid = true;
        float  m_GridStep = 32.f;

        int  m_HoverX = -1, m_HoverY = -1;
        bool m_HoverValid = false;

        int m_SelectedCellX = -1, m_SelectedCellY = -1;

        std::mutex m_Mutex;

        ImVec2 ScreenToCanvas(ImVec2 screen) const { ImVec2 p = screen - m_CanvasPos; return (p - m_Pan) / m_Zoom; }
        ImVec2 CanvasToScreen(ImVec2 canvas) const { return canvas * m_Zoom + m_Pan + m_CanvasPos; }
    };
}
