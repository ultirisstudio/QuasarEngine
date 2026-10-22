#include <quasar/core/Log.h>
#include <quasar/core/Timer.h>
#include <quasar/core/Window.h>

int main()
{
    Quasar::Log::Init();

    QUASAR_INFO("This is an info message");
    QUASAR_WARN("This is a warning message");
    QUASAR_ERROR("This is an error message at {} on {}", __FILE__, __LINE__);
    QUASAR_TRACE("This is a trace message");

    Quasar::Timer timer;
    QUASAR_INFO("Timer just created: {:.6f} ms", timer.ElapsedMilliseconds());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QUASAR_INFO("After sleeping 100 ms: {:.3f} ms / {:.6f} s", timer.ElapsedMilliseconds(), timer.ElapsedSeconds());

    timer.Reset();
    QUASAR_INFO("Right after Reset: {:.6f} ms", timer.ElapsedMilliseconds());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    QUASAR_INFO("After sleeping 250 ms: {:.3f} ms", timer.ElapsedMilliseconds());

    Quasar::Scope<Quasar::Window> window = Quasar::Window::Create({.Title = "Test Window", .Width = 800, .Height = 600, .VSync = true});

    while(!window->ShouldClose())
    {
        window->Update();
    }

    Quasar::Log::Shutdown();

    return 0;
}
