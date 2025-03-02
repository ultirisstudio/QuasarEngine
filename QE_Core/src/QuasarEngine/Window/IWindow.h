#pragma once

class IWindow {
public:
    virtual ~IWindow() = default;

    virtual void Initialize() = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual void Run() = 0;
    virtual void Close() = 0;
};