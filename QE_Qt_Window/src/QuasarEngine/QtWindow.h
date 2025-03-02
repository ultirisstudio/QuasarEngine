#pragma once

#include "QuasarEngine/Window/IWindow.h"

class QApplication;
class QWidget;

class QtWindow : public IWindow {
public:
    QtWindow(int width, int height, const char* title, int argc, char* argv[]);
    ~QtWindow();

    void Initialize() override;
    void SwapBuffers() override;
    void PollEvents() override;
    void Run() override;
    void Close() override;

    static void Register(int argc, char* argv[]);

private:
    //QApplication* app;
    //QWidget* window;
};
