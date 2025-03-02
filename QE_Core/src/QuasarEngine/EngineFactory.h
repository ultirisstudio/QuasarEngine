#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include "Renderer/IRenderer.h"
#include "GUI/IGUI.h"
#include "Window/IWindow.h"

enum class RendererAPI { OpenGL, Vulkan, DirectX };
enum class GUIAPI { ImGui, Qt };
enum class WindowAPI { GLFW, Qt };

class EngineFactory {
public:
    using RendererCreator = std::function<std::unique_ptr<IRenderer>()>;
    using GUICreator = std::function<std::unique_ptr<IGUI>()>;
    using WindowCreator = std::function<std::unique_ptr<IWindow>()>;

    static EngineFactory& Instance();

    void RegisterRenderer(RendererAPI api, RendererCreator creator);
    void RegisterGUI(GUIAPI api, GUICreator creator);
    void RegisterWindow(WindowAPI api, WindowCreator creator);

    std::unique_ptr<IRenderer> CreateRenderer(RendererAPI api);
    std::unique_ptr<IGUI> CreateGUI(GUIAPI api);
    std::unique_ptr<IWindow> CreateWindow(WindowAPI api);

private:
    EngineFactory() = default;
    std::unordered_map<RendererAPI, RendererCreator> rendererCreators;
    std::unordered_map<GUIAPI, GUICreator> guiCreators;
    std::unordered_map<WindowAPI, WindowCreator> windowCreators;
};
