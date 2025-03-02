#pragma once

#include <QuasarEngine/EngineFactory.h>
#include <QuasarEngine/GUI/IGUI.h>

#include <iostream>

class QtGUI : public IGUI
{
public:
    QtGUI() = default;
    ~QtGUI() override
    {
        Shutdown();
    }

    void Initialize() override
    {
        std::cout << "Initializing Qt GUI" << std::endl;
    }

    void Render() override
    {
        std::cout << "Rendering Qt GUI" << std::endl;
    }

    void Shutdown() override
    {
        std::cout << "Shutting down Qt GUI" << std::endl;
    }

    static void Register()
    {
        EngineFactory::Instance().RegisterGUI(GUIAPI::Qt, [] {
            return std::make_unique<QtGUI>();
        });
    }
};