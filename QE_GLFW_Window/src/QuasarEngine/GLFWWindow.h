#pragma once

#include "QuasarEngine/Window/IWindow.h"

#include <string>

struct GLFWwindow;

class GLFWWindow : public IWindow {
public:
    GLFWWindow(int width, int height, const std::string& title);
    ~GLFWWindow();

    void Initialize() override;
    void SwapBuffers() override;
    void PollEvents() override;
    void Run() override;
    void Close() override;

    bool ShouldClose() const;
    GLFWwindow* GetNativeWindow() const { return window; }

    static void Register();

private:
    int width;
    int height;
    std::string title;
    GLFWwindow* window = nullptr;
};
