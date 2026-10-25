#include <quasar/core/EntryPoint.h>
#include <quasar/core/Log.h>

#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv)
{
    Quasar::Log::Init();

    int exitCode = 0;
    {
        const std::span<char*> commandLine(argv, static_cast<std::size_t>(argc));
        std::vector<std::string> arguments;
        if (!commandLine.empty())
        {
            arguments.assign(commandLine.begin() + 1, commandLine.end());
        }

        Quasar::Scope<Quasar::Application> application = Quasar::CreateApplication(std::move(arguments));
        if (application)
        {
            application->Run();
        }
        else
        {
            QUASAR_ENGINE_ERROR("The application cannot be created");
            exitCode = 1;
        }
    }

    Quasar::Log::Shutdown();
    return exitCode;
}
