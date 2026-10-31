#pragma once

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/Log.h>
#include <quasar/core/Window.h>

#include <GLFW/glfw3.h>

namespace TestHelpers
{
    class ScopedLog
    {
    public:
        ScopedLog() { Quasar::Log::Init(); }
        ~ScopedLog() { Quasar::Log::Shutdown(); }

        ScopedLog(const ScopedLog&) = delete;
        ScopedLog& operator=(const ScopedLog&) = delete;
        ScopedLog(ScopedLog&&) = delete;
        ScopedLog& operator=(ScopedLog&&) = delete;
    };

    inline void UseNullPlatform()
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    inline Quasar::Scope<Quasar::Window> CreateNullWindow(const Quasar::WindowSpecification& specification = {})
    {
        UseNullPlatform();
        return Quasar::Window::Create(specification);
    }

    inline Quasar::Scope<Quasar::Application>
    CreateNullApplication(const Quasar::ApplicationSpecification& specification = {})
    {
        UseNullPlatform();
        return Quasar::Application::Create(specification);
    }

    inline void SendKey(Quasar::Window& window, int key, int action)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWkeyfun callback = glfwSetKeyCallback(handle, nullptr);
        glfwSetKeyCallback(handle, callback);
        callback(handle, key, 0, action, 0);
    }

    inline void SendCharacter(Quasar::Window& window, unsigned int codePoint)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWcharfun callback = glfwSetCharCallback(handle, nullptr);
        glfwSetCharCallback(handle, callback);
        callback(handle, codePoint);
    }

    inline void SendMouseButton(Quasar::Window& window, int button, int action)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWmousebuttonfun callback = glfwSetMouseButtonCallback(handle, nullptr);
        glfwSetMouseButtonCallback(handle, callback);
        callback(handle, button, action, 0);
    }

    inline void SendCursorPosition(Quasar::Window& window, double x, double y)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWcursorposfun callback = glfwSetCursorPosCallback(handle, nullptr);
        glfwSetCursorPosCallback(handle, callback);
        callback(handle, x, y);
    }

    inline void SendScroll(Quasar::Window& window, double xOffset, double yOffset)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWscrollfun callback = glfwSetScrollCallback(handle, nullptr);
        glfwSetScrollCallback(handle, callback);
        callback(handle, xOffset, yOffset);
    }

    inline void SendFocus(Quasar::Window& window, bool focused)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWwindowfocusfun callback = glfwSetWindowFocusCallback(handle, nullptr);
        glfwSetWindowFocusCallback(handle, callback);
        callback(handle, focused ? GLFW_TRUE : GLFW_FALSE);
    }

    inline void SendCloseRequest(Quasar::Window& window)
    {
        GLFWwindow* handle = window.GetNativeWindow();
        const GLFWwindowclosefun callback = glfwSetWindowCloseCallback(handle, nullptr);
        glfwSetWindowCloseCallback(handle, callback);
        callback(handle);
    }
}