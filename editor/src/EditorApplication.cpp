#include <quasar/core/Application.h>
#include <quasar/core/EntryPoint.h>

#include <string>
#include <utility>
#include <vector>

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    return Application::Create({.Name = "Editor", .Arguments = std::move(arguments), .Window = {.Title = "Editor"}});
}
