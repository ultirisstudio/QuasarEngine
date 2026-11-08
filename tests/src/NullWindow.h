#pragma once

#include "LogHelpers.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/KeyCodes.h>
#include <quasar/core/MouseCodes.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <cstdlib>

namespace TestHelpers
{
    // On Linux, the tests use the X11 driver of SDL, which supports the ImGui viewports, so they need an X server. The
    // CI runs them with xvfb-run.
    inline void UseTestVideoDriver()
    {
#ifdef QUASAR_PLATFORM_LINUX
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
        // The driver draws on the calling thread instead of helper threads, whose locks the thread sanitizer cannot
        // see, so it reports no false data races
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        setenv("LP_NUM_THREADS", "0", 1);
#endif
    }

    inline void DiscardPendingEvents()
    {
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    }

    inline Quasar::Scope<Quasar::Window> CreateTestWindow(Quasar::WindowSpecification specification)
    {
        UseTestVideoDriver();
        specification.Visible = false;
        Quasar::Scope<Quasar::Window> window = Quasar::Window::Create(specification);
        DiscardPendingEvents();
        return window;
    }

    inline Quasar::Scope<Quasar::Window> CreateNullWindow(Quasar::WindowSpecification specification = {})
    {
        specification.GraphicsContext = false;
        return CreateTestWindow(specification);
    }

    inline Quasar::Scope<Quasar::Window> CreateWindowWithContext(const Quasar::WindowSpecification& specification = {})
    {
        return CreateTestWindow(specification);
    }

    inline Quasar::Scope<Quasar::Application> CreateTestApplication(Quasar::ApplicationSpecification specification)
    {
        UseTestVideoDriver();
        specification.Window.Visible = false;
        Quasar::Scope<Quasar::Application> application = Quasar::Application::Create(specification);
        if (application)
        {
            application->GetGamepads().SetBackend(nullptr);
        }
        DiscardPendingEvents();
        return application;
    }

    inline Quasar::Scope<Quasar::Application> CreateNullApplication(Quasar::ApplicationSpecification specification = {})
    {
        specification.Window.GraphicsContext = false;
        return CreateTestApplication(specification);
    }

    inline Quasar::Scope<Quasar::Application> CreateApplicationWithContext(const Quasar::ApplicationSpecification& specification = {})
    {
        return CreateTestApplication(specification);
    }

    inline SDL_WindowID GetWindowId(const Quasar::Window& window)
    {
        return SDL_GetWindowID(window.GetNativeWindow());
    }

    inline void SendWindowEvent(const Quasar::Window& window, SDL_EventType type, std::int32_t data1 = 0, std::int32_t data2 = 0)
    {
        SDL_Event event{};
        event.type = type;
        event.window.windowID = GetWindowId(window);
        event.window.data1 = data1;
        event.window.data2 = data2;
        SDL_PushEvent(&event);
    }

    inline void SendKey(const Quasar::Window& window, Quasar::Key key, bool down, bool repeat = false)
    {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.windowID = GetWindowId(window);
        event.key.scancode = static_cast<SDL_Scancode>(key);
        event.key.key = SDL_GetKeyFromScancode(event.key.scancode, SDL_KMOD_NONE, false);
        event.key.down = down;
        event.key.repeat = repeat;
        SDL_PushEvent(&event);
    }

    inline void SendText(const Quasar::Window& window, const char* text)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_TEXT_INPUT;
        event.text.windowID = GetWindowId(window);
        event.text.text = text;
        SDL_PushEvent(&event);
    }

    inline void SendMouseButton(const Quasar::Window& window, Quasar::MouseButton button, bool down)
    {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.windowID = GetWindowId(window);
        event.button.button = static_cast<Uint8>(button);
        event.button.down = down;
        event.button.clicks = 1;
        const glm::vec2 position = window.GetMousePosition();
        event.button.x = position.x;
        event.button.y = position.y;
        SDL_PushEvent(&event);
    }

    inline void SendMouseMotion(const Quasar::Window& window, float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.windowID = GetWindowId(window);
        event.motion.x = x;
        event.motion.y = y;
        SDL_PushEvent(&event);
    }

    inline void SendScroll(const Quasar::Window& window, float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.windowID = GetWindowId(window);
        event.wheel.x = x;
        event.wheel.y = y;
        event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
        SDL_PushEvent(&event);
    }

    inline void SendFocus(const Quasar::Window& window, bool focused)
    {
        SendWindowEvent(window, focused ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST);
    }

    inline void SendCloseRequest(const Quasar::Window& window)
    {
        SendWindowEvent(window, SDL_EVENT_WINDOW_CLOSE_REQUESTED);
    }

    inline void SendResize(const Quasar::Window& window, std::int32_t width, std::int32_t height)
    {
        SendWindowEvent(window, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, width, height);
    }

    inline void SendMinimize(const Quasar::Window& window)
    {
        SendWindowEvent(window, SDL_EVENT_WINDOW_MINIMIZED);
    }

    inline void SendRestore(const Quasar::Window& window)
    {
        SendWindowEvent(window, SDL_EVENT_WINDOW_RESTORED);
    }
}
