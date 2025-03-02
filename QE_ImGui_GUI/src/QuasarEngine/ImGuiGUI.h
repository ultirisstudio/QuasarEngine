#pragma once

#include <QuasarEngine/EngineFactory.h>
#include <QuasarEngine/GUI/IGUI.h>

#include <iostream>

class ImGuiGUI : public IGUI
{
public:
    ImGuiGUI() = default;
    ~ImGuiGUI() override
    {
        Shutdown();
    }

    ImGuiGUI(const ImGuiGUI&) = delete;
    ImGuiGUI& operator=(const ImGuiGUI&) = delete;

    void Initialize() override
    {
        std::cout << "Initializing ImGui GUI" << std::endl;
    }

    void Render() override
    {
        std::cout << "Rendering ImGui GUI" << std::endl;
    }

    void Shutdown() override
    {
        std::cout << "Shutting down ImGui GUI" << std::endl;
    }

    static void Register()
    {
        EngineFactory::Instance().RegisterGUI(GUIAPI::ImGui, [] {
            return std::make_unique<ImGuiGUI>();
        });
    }
};