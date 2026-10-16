#include <quasar/Version.h>
#include <quasar/core/Log.h>

int main()
{
    Quasar::Log::Init();

    QUASAR_INFO("QuasarEditor v{}", "0.1.0");

    Quasar::Log::Shutdown();

    return 0;
}
