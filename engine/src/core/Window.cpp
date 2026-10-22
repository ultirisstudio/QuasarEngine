#include <quasar/core/Log.h>
#include <quasar/core/Window.h>

#include <GLFW/glfw3.h>

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
    }

    Window::Window(GLFWwindow* handle, const WindowSpecification& specification)
        : m_Handle(handle), m_Title(specification.Title)
    {
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

        GLFWwindow* handle = glfwCreateWindow(static_cast<int>(specification.Width),
                                              static_cast<int>(specification.Height),
                                              specification.Title.c_str(),
                                              nullptr,
                                              nullptr);
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

        QUASAR_ENGINE_INFO("Created the window '{}' ({}x{})", window->GetTitle(), window->GetWidth(), window->GetHeight());
        return window;
    }

    void Window::Update()
    {
        glfwSwapBuffers(m_Handle);
        glfwPollEvents();
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
}