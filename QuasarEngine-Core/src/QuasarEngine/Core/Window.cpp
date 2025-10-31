#include "qepch.h"

#include "QuasarEngine/Core/Window.h"
#include "QuasarEngine/Events/ApplicationEvent.h"
#include "QuasarEngine/Events/MouseEvent.h"
#include "QuasarEngine/Events/KeyEvent.h"
#include "QuasarEngine/Renderer/RendererAPI.h"
#include "Logger.h"

#include <GLFW/glfw3.h>

/*#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

static void EnableWinBorderlessResizeAndShadow(GLFWwindow* win)
{
    HWND hwnd = glfwGetWin32Window(win);
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    style &= ~WS_CAPTION;

    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    const int DWMWCP_ROUND = 2;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &DWMWCP_ROUND, sizeof(int));

    const int DWMNCRP_ENABLED = 2;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &DWMNCRP_ENABLED, sizeof(int));
}
#endif*/

namespace QuasarEngine
{
    int Window::s_GLFWRefCount = 0;

    static void GLFWErrorCallback(int error, const char* description)
    {
        Q_ERROR("GLFW Error (" + std::to_string(error) + std::string("): ") + std::string(description));
    }

    Window::Window()
    {
        if (s_GLFWRefCount == 0) {
            glfwSetErrorCallback(GLFWErrorCallback);
            const int ok = glfwInit();
            if (!ok) {
                Q_ERROR("glfwInit() failed");
                Q_ASSERT(false, "GLFW init failed");
            }
        }
        ++s_GLFWRefCount;

        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            break;
        case RendererAPI::API::Vulkan:
        case RendererAPI::API::DirectX:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            break;
        default:
            Q_ERROR("Unsupported RendererAPI selected!");
            break;
        }

        m_Window = glfwCreateWindow(static_cast<int>(m_Data.Width),
            static_cast<int>(m_Data.Height),
            m_Data.Title.c_str(),
            nullptr, nullptr);
        if (!m_Window) {
            Q_ERROR("glfwCreateWindow() failed");
            Q_ASSERT(false, "Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(m_Window, &m_Data);

//#if defined(_WIN32)
//        EnableWinBorderlessResizeAndShadow(m_Window);
//#endif

        m_Context = GraphicsContext::Create(m_Window);
        Q_ASSERT(m_Context.get(), "GraphicsContext::Create failed");

        SetVSync(false);

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                if (!data.EventCallback) return;
                WindowCloseEvent event;
                data.EventCallback(event);
            });

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                data.Width = static_cast<unsigned int>(width);
                data.Height = static_cast<unsigned int>(height);
                if (!data.EventCallback) return;
                WindowResizeEvent event(width, height);
                data.EventCallback(event);
            });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int /*mods*/)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                if (!data.EventCallback) return;
                if (action == GLFW_PRESS) {
                    MouseButtonPressedEvent event(button);
                    data.EventCallback(event);
                }
                else if (action == GLFW_RELEASE) {
                    MouseButtonReleasedEvent event(button);
                    data.EventCallback(event);
                }
            });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                if (!data.EventCallback) return;
                MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
                data.EventCallback(event);
            });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                data.MousePos = glm::uvec2(static_cast<unsigned int>(xPos), static_cast<unsigned int>(yPos));
                if (!data.EventCallback) return;
                MouseMovedEvent event(xPos, yPos);
                data.EventCallback(event);
            });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                if (!data.EventCallback) return;

                switch (action)
                {
                case GLFW_PRESS: {
                    KeyPressedEvent event(key, 0);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(key);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    KeyPressedEvent event(key, true);
                    data.EventCallback(event);
                    break;
                }
                default: break;
                }
            });

        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
            {
                auto& data = *static_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
                if (!data.EventCallback) return;
                KeyTypedEvent event(keycode);
                data.EventCallback(event);
            });
    }

    Window::~Window()
    {
        Shutdown();
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::WaitEvents(double timeoutSeconds)
    {
        if (timeoutSeconds > 0.0)
            glfwWaitEventsTimeout(timeoutSeconds);
        else
            glfwWaitEvents();
    }

    void Window::BeginFrame()
    {
        m_Context->BeginFrame();
    }

    void Window::EndFrame()
    {
        m_Context->EndFrame();
    }

    void Window::Resize(unsigned int width, unsigned int height)
    {
        m_Data.Width = width;
        m_Data.Height = height;
        m_Context->Resize(width, height);
    }

/*#if defined(_WIN32)
    void Window::ApplyDwmFrameColors(DWORD borderColor, DWORD captionColor, DWORD textColor)
    {
        HWND hwnd = glfwGetWin32Window(m_Window);
        (void)DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
        (void)DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        (void)DwmSetWindowAttribute(hwnd, (DWMWINDOWATTRIBUTE)DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
    }
#endif*/

    void Window::Shutdown()
    {
        if (m_Window) {
            glfwSetWindowUserPointer(m_Window, nullptr);
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
        if (s_GLFWRefCount > 0) {
            --s_GLFWRefCount;
            if (s_GLFWRefCount == 0) {
                glfwTerminate();
            }
        }
    }

    void Window::SetVSync(bool enabled)
    {
        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) {
            glfwMakeContextCurrent(m_Window);
            glfwSwapInterval(enabled ? 1 : 0);
        }
        
        // TODO: ajouté SetVSync() côté GraphicsContext pour Vulkan/DirectX.

        m_Data.VSync = enabled;
    }

    void Window::SetInputMode(bool cursorDisabled, bool rawMouseMotion)
    {
        if (cursorDisabled) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(
                m_Window,
                GLFW_RAW_MOUSE_MOTION,
                (rawMouseMotion && glfwRawMouseMotionSupported()) ? GLFW_TRUE : GLFW_FALSE
            );
        }
        else {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
    }

    void Window::SetCursorVisibility(bool visible)
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    void Window::SetMousePosition(float x, float y)
    {
        glfwSetCursorPos(m_Window, static_cast<double>(x), static_cast<double>(y));
        m_Data.MousePos = glm::uvec2(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
    }

    bool Window::WaitEventsTimeout(double seconds)
    {
        if (seconds < 0.0) seconds = 0.0;
        glfwWaitEventsTimeout(seconds);
        return true;
    }

    void Window::Minimize() { glfwIconifyWindow(m_Window); }

    void Window::Maximize() { glfwMaximizeWindow(m_Window); }

    void Window::Restore() { glfwRestoreWindow(m_Window); }

    bool Window::IsMaximized() const
    {
        return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    void Window::ToggleMaximize()
    {
        if (IsMaximized()) Restore(); else Maximize();
    }

    void Window::SetPosition(int x, int y)
    {
        glfwSetWindowPos(m_Window, x, y);
    }

    void Window::GetPosition(int& x, int& y) const
    {
        glfwGetWindowPos(m_Window, &x, &y);
    }

    void Window::MoveBy(int dx, int dy)
    {
        int x, y; GetPosition(x, y);
        SetPosition(x + dx, y + dy);
    }

    void Window::SetDecorated(bool decorated)
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, decorated ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetResizable(bool resizable)
    {
        glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetFloating(bool floating)
    {
        glfwSetWindowAttrib(m_Window, GLFW_FLOATING, floating ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::SetOpacity(float alpha)
    {
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        glfwSetWindowOpacity(m_Window, alpha);
    }

    void Window::SetSizeLimits(int minW, int minH, int maxW, int maxH)
    {
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR==3 && GLFW_VERSION_MINOR>=3)
        const int GC = GLFW_DONT_CARE;
#else
        const int GC = INT_MAX;
#endif
        if (maxW <= 0) maxW = GC;
        if (maxH <= 0) maxH = GC;
        glfwSetWindowSizeLimits(m_Window, minW, minH, maxW, maxH);
    }

    void Window::SetAspectRatio(int numer, int denom)
    {
        if (numer <= 0 || denom <= 0) {
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR==3 && GLFW_VERSION_MINOR>=3)
            glfwSetWindowAspectRatio(m_Window, GLFW_DONT_CARE, GLFW_DONT_CARE);
#else
            
#endif
        }
        else {
            glfwSetWindowAspectRatio(m_Window, numer, denom);
        }
    }

    void Window::CenterOnPrimaryMonitor()
    {
        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        if (!mon) return;

#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR==3 && GLFW_VERSION_MINOR>=3)
        int mx, my, mw, mh;
        glfwGetMonitorWorkarea(mon, &mx, &my, &mw, &mh);
#else
        const GLFWvidmode* vm = glfwGetVideoMode(mon);
        int mx = 0, my = 0, mw = vm ? vm->width : (int)m_Data.Width, mh = vm ? vm->height : (int)m_Data.Height;
#endif

        int x = mx + (mw - (int)m_Data.Width) / 2;
        int y = my + (mh - (int)m_Data.Height) / 2;
        SetPosition(x, y);
    }

    void Window::ToggleMaximizeWorkArea()
    {
        if (!m_CustomMaximized)
        {
            glfwGetWindowPos(m_Window, &m_PrevX, &m_PrevY);
            int ww, wh; glfwGetWindowSize(m_Window, &ww, &wh);
            m_PrevW = ww; m_PrevH = wh;

            GLFWmonitor* mon = glfwGetPrimaryMonitor();
            if (mon) {
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR==3 && GLFW_VERSION_MINOR>=3)
                int mx, my, mw, mh; glfwGetMonitorWorkarea(mon, &mx, &my, &mw, &mh);
#else
                const GLFWvidmode* vm = glfwGetVideoMode(mon);
                int mx = 0, my = 0, mw = vm ? vm->width : ww, mh = vm ? vm->height : wh;
#endif
                glfwSetWindowPos(m_Window, mx, my);
                glfwSetWindowSize(m_Window, mw, mh);
            }
            m_CustomMaximized = true;
        }
        else
        {
            glfwSetWindowPos(m_Window, m_PrevX, m_PrevY);
            glfwSetWindowSize(m_Window, m_PrevW, m_PrevH);
            m_CustomMaximized = false;
        }
    }

    void Window::SetTitle(const std::string& title)
    {
        m_Data.Title = title;
        glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
    }
}
