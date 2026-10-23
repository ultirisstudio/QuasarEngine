#include <quasar/core/Log.h>
#include <quasar/core/Window.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/WindowEvent.h>

#include <GLFW/glfw3.h>

#include <utility>

namespace Quasar
{
    namespace
    {
        std::uint32_t s_WindowCount = 0;

        void OnGlfwError(int code, const char* description)
        {
            QUASAR_ENGINE_ERROR("GLFW error {}: {}", code, description);
        }

        const char* GetPlatformName()
        {
            switch (glfwGetPlatform())
            {
                case GLFW_PLATFORM_WIN32:
                    return "Win32";
                case GLFW_PLATFORM_X11:
                    return "X11";
                case GLFW_PLATFORM_WAYLAND:
                    return "Wayland";
                default:
                    return "unknown";
            }
        }

        Window& GetWindow(GLFWwindow* handle)
        {
            Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
            QUASAR_ENGINE_ASSERT(window != nullptr, "GLFW window has no user pointer");
            return *window;
        }
    }

    Window::Window(GLFWwindow* handle, const WindowSpecification& specification)
        : m_Handle(handle), m_Title(specification.Title)
    {
        glfwSetWindowUserPointer(handle, this);
        SetGlfwCallbacks();
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Handle);

        --s_WindowCount;
        if (s_WindowCount == 0)
        {
            glfwTerminate();
        }
    }

    Scope<Window> Window::Create(const WindowSpecification& specification)
    {
        if (s_WindowCount == 0)
        {
            glfwSetErrorCallback(OnGlfwError);
            if (glfwInit() == GLFW_FALSE)
            {
                QUASAR_ENGINE_ERROR("Cannot create the window '{}': GLFW cannot be initialized", specification.Title);
                return nullptr;
            }

            QUASAR_ENGINE_TRACE("GLFW {}", glfwGetVersionString());
            QUASAR_ENGINE_INFO("Window platform: {}", GetPlatformName());
        }

        GLFWwindow* handle =
            glfwCreateWindow(static_cast<int>(specification.Width), static_cast<int>(specification.Height),
                             specification.Title.c_str(), nullptr, nullptr);
        if (handle == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the window '{}'", specification.Title);
            if (s_WindowCount == 0)
            {
                glfwTerminate();
            }
            return nullptr;
        }

        ++s_WindowCount;
        glfwMakeContextCurrent(handle);

        Scope<Window> window(new Window(handle, specification));
        window->SetVSync(specification.VSync);

        QUASAR_ENGINE_INFO("Created the window '{}' ({}x{})", window->GetTitle(), window->GetWidth(),
                           window->GetHeight());
        return window;
    }

    void Window::Update()
    {
        glfwSwapBuffers(m_Handle);
        glfwPollEvents();
    }

    void Window::SetEventCallback(EventCallback callback)
    {
        m_EventCallback = std::move(callback);
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_Handle) == GLFW_TRUE;
    }

    void Window::Close()
    {
        glfwSetWindowShouldClose(m_Handle, GLFW_TRUE);
    }

    void Window::SetVSync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
        m_VSync = enabled;
    }

    bool Window::IsVSync() const
    {
        return m_VSync;
    }

    std::uint32_t Window::GetWidth() const
    {
        int width = 0;
        glfwGetFramebufferSize(m_Handle, &width, nullptr);
        return static_cast<std::uint32_t>(width);
    }

    std::uint32_t Window::GetHeight() const
    {
        int height = 0;
        glfwGetFramebufferSize(m_Handle, nullptr, &height);
        return static_cast<std::uint32_t>(height);
    }

    const std::string& Window::GetTitle() const
    {
        return m_Title;
    }

    GLFWwindow* Window::GetNativeWindow() const
    {
        return m_Handle;
    }

    std::string Window::GetKeyName(Key key) const
    {
        const char* name = glfwGetKeyName(static_cast<int>(key), 0);
        return name != nullptr ? std::string(name) : std::string();
    }

    void Window::SetGlfwCallbacks()
    {
        glfwSetFramebufferSizeCallback(m_Handle, [](GLFWwindow* handle, int width, int height)
        {
            Window& window = GetWindow(handle);
            WindowResizeEvent event(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
            window.SendEvent(event);
        });

        glfwSetWindowCloseCallback(m_Handle, [](GLFWwindow* handle)
        {
            Window& window = GetWindow(handle);
            WindowCloseEvent event;
            window.SendEvent(event);
        });

        glfwSetWindowFocusCallback(m_Handle, [](GLFWwindow* handle, int focused)
        {
            Window& window = GetWindow(handle);
            WindowFocusEvent event(focused == GLFW_TRUE);
            window.SendEvent(event);
        });

        glfwSetKeyCallback(m_Handle, [](GLFWwindow* handle, int key, int, int action, int)
        {
            if (key == GLFW_KEY_UNKNOWN)
            {
                return;
            }

            Window& window = GetWindow(handle);
            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(static_cast<Key>(key), false);
                    window.SendEvent(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(static_cast<Key>(key));
                    window.SendEvent(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(static_cast<Key>(key), true);
                    window.SendEvent(event);
                    break;
                }
            }
        });

        glfwSetCharCallback(m_Handle, [](GLFWwindow* handle, unsigned int codePoint)
        {
            Window& window = GetWindow(handle);
            KeyTypedEvent event(static_cast<char32_t>(codePoint));
            window.SendEvent(event);
        });
    }

    void Window::SendEvent(Event& event)
    {
        if (m_EventCallback)
        {
            m_EventCallback(event);
        }
    }
}
