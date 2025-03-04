#pragma once

#include "QuasarEngine/Window/IWindow.h"

#include <string>

struct GLFWwindow;

class GlfwWindow : public IWindow {
public:
    GlfwWindow();
    ~GlfwWindow();

    void Initialize(unsigned int width, unsigned int height, const std::string& title) override;
    void SwapBuffers() override;
    void PollEvents() override;
    void Run() override;
    void Close() override;

    bool ShouldClose() const;
    GLFWwindow* GetNativeWindow() const { return window; }

    static void Register();

private:
    GLFWwindow* window = nullptr;
};
