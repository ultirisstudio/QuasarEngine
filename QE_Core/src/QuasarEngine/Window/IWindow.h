#pragma once

#include <string>

class IWindow {
public:
    virtual ~IWindow() = default;

    virtual void Initialize(unsigned int width = 800, unsigned int height = 600, const std::string& title = "Main Window") = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual void Run() = 0;
    virtual void Close() = 0;
};