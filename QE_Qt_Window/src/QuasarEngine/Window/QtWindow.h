#pragma once

#include "QuasarEngine/Window/IWindow.h"

class QApplication;
class QWidget;

class QtWindow : public IWindow {
public:
    QtWindow(int width, int height, const char* title);
    ~QtWindow();

    void Initialize() override;
    void SwapBuffers() override;
    void PollEvents() override;
    void Run() override;
    void Close() override;

    static void Register();

private:
    //QApplication* app;
    //QWidget* window;
};
