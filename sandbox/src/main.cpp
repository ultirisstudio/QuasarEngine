#include <quasar/core/Log.h>

int main()
{
    Quasar::Log::Init();

    QUASAR_INFO("This is an info message");
    QUASAR_WARN("This is a warning message");
    QUASAR_ERROR("This is an error message at {} on {}", __FILE__, __LINE__);
    QUASAR_TRACE("This is a trace message");

    Quasar::Log::Shutdown();

    return 0;
}
