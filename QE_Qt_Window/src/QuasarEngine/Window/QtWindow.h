#pragma once

#include "QuasarEngine/Window/IWindow.h"

class QApplication;
class QWidget;

class QtWindow : public IWindow {
public:
    QtWindow();
    ~QtWindow();

    void Initialize(unsigned int width, unsigned int height, const std::string& title) override;
    void SwapBuffers() override;
    void PollEvents() override;
    void Run() override;
    void Close() override;

    static void Register();
};
