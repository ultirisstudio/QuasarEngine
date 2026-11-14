#include <quasar/core/Window.h>
#include <quasar/events/AppEvent.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/TouchEvent.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/log/Log.h>

#include <renderer/GraphicsContextFactory.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>
#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr const char* WindowProperty = "Quasar.Window";

        constexpr SDL_Keycode FirstPrintedKey = 0x21;
        constexpr SDL_Keycode DeleteKey = 0x7F;

        // The video support of SDL is started with the first window and stopped with the last one
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::uint32_t s_WindowCount = 0;

        Window* FindWindow(const SDL_Event& event)
        {
            SDL_Window* handle = SDL_GetWindowFromEvent(&event);
            if (handle == nullptr)
            {
                return nullptr;
            }
            return static_cast<Window*>(SDL_GetPointerProperty(SDL_GetWindowProperties(handle), WindowProperty, nullptr));
        }

        // SDL also turns the touches and the pens into mouse events, and the mouse into touches. Only the real mouse
        // sends mouse events, and only the screen and the pens send touch events.
        bool IsMouse(SDL_MouseID mouse)
        {
            return mouse != SDL_TOUCH_MOUSEID && mouse != SDL_PEN_MOUSEID;
        }

        bool IsTouchScreen(SDL_TouchID touch)
        {
            return touch != SDL_MOUSE_TOUCHID;
        }

        glm::vec2 GetTouchPosition(SDL_Window* window, const SDL_TouchFingerEvent& finger)
        {
            int width = 0;
            int height = 0;
            SDL_GetWindowSize(window, &width, &height);
            return {finger.x * static_cast<float>(width), finger.y * static_cast<float>(height)};
        }

        std::uint32_t ButtonMask(MouseButton button)
        {
            const auto index = static_cast<std::uint32_t>(button);
            return index >= 1 && index <= 32 ? SDL_BUTTON_MASK(index) : 0;
        }
    }

    Window::Window(SDL_Window* handle, const WindowSpecification& specification) : m_Handle(handle), m_Title(specification.Title)
    {
        SDL_SetPointerProperty(SDL_GetWindowProperties(handle), WindowProperty, this);

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(handle, &width, &height);
        m_Width = static_cast<std::uint32_t>(width);
        m_Height = static_cast<std::uint32_t>(height);
    }

    Window::~Window()
    {
        m_Context.reset();
        SDL_DestroyWindow(m_Handle);

        --s_WindowCount;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    Scope<Window> Window::Create(const WindowSpecification& specification)
    {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            QUASAR_ENGINE_ERROR("Cannot create the window '{}': the video support of SDL cannot start: {}", specification.Title, SDL_GetError());
            return nullptr;
        }

        if (s_WindowCount == 0)
        {
            const int version = SDL_GetVersion();
            QUASAR_ENGINE_TRACE("SDL {}.{}.{}", SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version), SDL_VERSIONNUM_MICRO(version));
            QUASAR_ENGINE_INFO("Video driver: {}", SDL_GetCurrentVideoDriver());
        }

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (!specification.Visible)
        {
            flags |= SDL_WINDOW_HIDDEN;
        }
        flags |= PrepareGraphicsWindow(specification.Graphics);

        SDL_Window* handle =
            SDL_CreateWindow(specification.Title.c_str(), static_cast<int>(specification.Width), static_cast<int>(specification.Height), flags);
        if (handle == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the window '{}': {}", specification.Title, SDL_GetError());
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            return nullptr;
        }

        ++s_WindowCount;

        Scope<Window> window(new Window(handle, specification));

        if (specification.Graphics != GraphicsAPI::None)
        {
            window->m_Context = CreateGraphicsContext(specification.Graphics, handle);
            if (window->m_Context == nullptr)
            {
                QUASAR_ENGINE_ERROR("Cannot create the window '{}': it has no usable graphics context", specification.Title);
                return nullptr;
            }
        }

        window->SetVSync(specification.VSync);
        if constexpr (!PlatformDetection::IsAndroid())
        {
            SDL_StartTextInput(handle);
        }

        QUASAR_ENGINE_INFO("Created the window '{}' ({}x{})", window->GetTitle(), window->GetWidth(), window->GetHeight());
        return window;
    }

    void Window::Update()
    {
        if (m_Context)
        {
            m_Context->SwapBuffers();
        }

        if (!PlatformDetection::IsAndroid() && !SDL_TextInputActive(m_Handle))
        {
            SDL_StartTextInput(m_Handle);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (m_NativeEventCallback)
            {
                m_NativeEventCallback(event);
            }

            Window* window = FindWindow(event);
            (window != nullptr ? window : this)->ProcessEvent(event);
        }
    }

    void Window::SetEventCallback(EventCallback callback)
    {
        m_EventCallback = std::move(callback);
    }

    void Window::SetNativeEventCallback(NativeEventCallback callback)
    {
        m_NativeEventCallback = std::move(callback);
    }

    bool Window::ShouldClose() const
    {
        return m_ShouldClose;
    }

    void Window::Close()
    {
        m_ShouldClose = true;
    }

    void Window::SetVSync(bool enabled)
    {
        if (m_Context)
        {
            m_Context->SetVSync(enabled);
        }
        m_VSync = enabled;
    }

    bool Window::IsVSync() const
    {
        return m_VSync;
    }

    std::uint32_t Window::GetWidth() const
    {
        return m_Width;
    }

    std::uint32_t Window::GetHeight() const
    {
        return m_Height;
    }

    const std::string& Window::GetTitle() const
    {
        return m_Title;
    }

    bool Window::IsKeyDown(Key key) const
    {
        const auto index = static_cast<std::size_t>(key);
        return index < KeyCount && m_KeysDown.test(index);
    }

    bool Window::IsMouseButtonDown(MouseButton button) const
    {
        return (m_MouseButtonsDown & ButtonMask(button)) != 0;
    }

    glm::vec2 Window::GetMousePosition() const
    {
        return m_MousePosition;
    }

    SDL_Window* Window::GetNativeWindow() const
    {
        return m_Handle;
    }

    GraphicsAPI Window::GetGraphicsAPI() const
    {
        return m_Context ? m_Context->GetAPI() : GraphicsAPI::None;
    }

    GraphicsContext* Window::GetGraphicsContext() const
    {
        return m_Context.get();
    }

    // Not static on purpose: the video support of SDL must be started, which an existing window guarantees
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    std::string Window::GetKeyName(Key key) const
    {
        const SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(key), SDL_KMOD_NONE, false);
        if ((keycode & SDLK_SCANCODE_MASK) != 0 || keycode < FirstPrintedKey || keycode == DeleteKey)
        {
            return {};
        }

        std::array<char, 5> text{};
        SDL_UCS4ToUTF8(keycode, text.data());
        return {text.data()};
    }

    void Window::ProcessEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                m_ShouldClose = true;
                WindowCloseEvent closeEvent;
                SendEvent(closeEvent);
                break;
            }
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                m_Width = static_cast<std::uint32_t>(event.window.data1);
                m_Height = static_cast<std::uint32_t>(event.window.data2);
                WindowResizeEvent resizeEvent(m_Width, m_Height);
                SendEvent(resizeEvent);
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                WindowFocusEvent focusEvent(event.type == SDL_EVENT_WINDOW_FOCUS_GAINED);
                SendEvent(focusEvent);
                break;
            }
            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                if (!m_Minimized)
                {
                    m_Minimized = true;
                    WindowMinimizeEvent minimizeEvent(true);
                    SendEvent(minimizeEvent);
                }
                break;
            }
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
            {
                if (m_Minimized)
                {
                    m_Minimized = false;
                    WindowMinimizeEvent minimizeEvent(false);
                    SendEvent(minimizeEvent);
                }
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                const auto index = static_cast<std::size_t>(event.key.scancode);
                if (event.key.scancode == SDL_SCANCODE_UNKNOWN || index >= KeyCount)
                {
                    break;
                }

                const auto key = static_cast<Key>(event.key.scancode);
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    m_KeysDown.set(index);
                    KeyPressedEvent keyEvent(key, event.key.repeat);
                    SendEvent(keyEvent);
                }
                else
                {
                    m_KeysDown.reset(index);
                    KeyReleasedEvent keyEvent(key);
                    SendEvent(keyEvent);
                }
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                const char* text = event.text.text;
                std::size_t length = SDL_strlen(text);
                while (length > 0)
                {
                    const Uint32 codePoint = SDL_StepUTF8(&text, &length);
                    KeyTypedEvent typedEvent(static_cast<char32_t>(codePoint));
                    SendEvent(typedEvent);
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
            {
                if (!IsMouse(event.motion.which))
                {
                    break;
                }
                m_MousePosition = {event.motion.x, event.motion.y};
                MouseMovedEvent movedEvent(event.motion.x, event.motion.y);
                SendEvent(movedEvent);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                if (!IsMouse(event.button.which))
                {
                    break;
                }
                const auto button = static_cast<MouseButton>(event.button.button);
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    m_MouseButtonsDown |= ButtonMask(button);
                    MouseButtonPressedEvent buttonEvent(button);
                    SendEvent(buttonEvent);
                }
                else
                {
                    m_MouseButtonsDown &= ~ButtonMask(button);
                    MouseButtonReleasedEvent buttonEvent(button);
                    SendEvent(buttonEvent);
                }
                break;
            }
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_MOTION:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED:
            {
                ProcessTouchEvent(event);
                break;
            }
            case SDL_EVENT_TERMINATING:
            {
                AppTerminatingEvent terminatingEvent;
                SendEvent(terminatingEvent);
                break;
            }
            case SDL_EVENT_LOW_MEMORY:
            {
                AppLowMemoryEvent lowMemoryEvent;
                SendEvent(lowMemoryEvent);
                break;
            }
            case SDL_EVENT_DID_ENTER_BACKGROUND:
            case SDL_EVENT_DID_ENTER_FOREGROUND:
            {
                AppBackgroundEvent backgroundEvent(event.type == SDL_EVENT_DID_ENTER_BACKGROUND);
                SendEvent(backgroundEvent);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
            {
                const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
                MouseScrolledEvent scrolledEvent(event.wheel.x * direction, event.wheel.y * direction);
                SendEvent(scrolledEvent);
                break;
            }
            default:
                break;
        }
    }

    void Window::ProcessTouchEvent(const SDL_Event& event)
    {
        const SDL_TouchFingerEvent& finger = event.tfinger;
        if (!IsTouchScreen(finger.touchID))
        {
            return;
        }

        const glm::vec2 position = GetTouchPosition(m_Handle, finger);
        if (event.type == SDL_EVENT_FINGER_DOWN)
        {
            TouchDownEvent touchEvent(finger.fingerID, position, finger.pressure);
            SendEvent(touchEvent);
        }
        else if (event.type == SDL_EVENT_FINGER_MOTION)
        {
            TouchMovedEvent touchEvent(finger.fingerID, position, finger.pressure);
            SendEvent(touchEvent);
        }
        else
        {
            TouchUpEvent touchEvent(finger.fingerID, position, finger.pressure);
            SendEvent(touchEvent);
        }
    }

    void Window::SendEvent(Event& event)
    {
        if (m_EventCallback)
        {
            m_EventCallback(event);
        }
    }
}
