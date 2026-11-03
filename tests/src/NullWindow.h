#pragma once

#include "LogHelpers.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>

#include <GLFW/glfw3.h>

#include <cstdlib>

namespace TestHelpers
{
    inline void UseNullPlatform()
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
        glfwInit();
    }

    inline Quasar::Scope<Quasar::Window> CreateNullWindow(Quasar::WindowSpecification specification = {})
    {
        UseNullPlatform();
        specification.GraphicsContext = false;
        return Quasar::Window::Create(specification);
    }

    inline Quasar::Scope<Quasar::Application> CreateNullApplication(Quasar::ApplicationSpecification specification = {})
    {
        UseNullPlatform();
        specification.Window.GraphicsContext = false;
        return Quasar::Application::Create(specification);
    }

#ifdef QUASAR_PLATFORM_LINUX
    inline void UseSoftwareOpenGL()
    {
        UseNullPlatform();
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        // The driver draws on the calling thread instead of helper threads, whose locks the thread sanitizer cannot
        // see, so it reports no false data races
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        setenv("LP_NUM_THREADS", "0", 1);
    }

    inline Quasar::Scope<Quasar::Window> CreateNullWindowWithContext(const Quasar::WindowSpecification& specification = {})
    {
        UseSoftwareOpenGL();
        return Quasar::Window::Create(specification);
    }

    inline Quasar::Scope<Quasar::Application> CreateNullApplicationWithContext(const Quasar::ApplicationSpecification& specification = {})
    {
        UseSoftwareOpenGL();
        return Quasar::Application::Create(specification);
    }
#endif

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