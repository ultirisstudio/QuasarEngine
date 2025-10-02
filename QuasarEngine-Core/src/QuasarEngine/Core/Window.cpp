#include "qepch.h"

#include "QuasarEngine/Core/Window.h"
#include "QuasarEngine/Events/ApplicationEvent.h"
#include "QuasarEngine/Events/MouseEvent.h"
#include "QuasarEngine/Events/KeyEvent.h"
#include "QuasarEngine/Renderer/Renderer.h"
#include "Logger.h"

#include <GLFW/glfw3.h>

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

    void Window::SetMaximized(bool maximized)
    {
        if (maximized)
            glfwMaximizeWindow(m_Window);
        else
            glfwRestoreWindow(m_Window);
    }

    void Window::SetTitle(const std::string& title)
    {
        m_Data.Title = title;
        glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
    }
}
