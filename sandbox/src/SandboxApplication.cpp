#include "SandboxLayer.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/EntryPoint.h>

#include <string>
#include <utility>
#include <vector>

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    Quasar::Scope<Application> application = Application::Create(
        {.Name = "Sandbox", .Arguments = std::move(arguments), .Window = {.Title = "Sandbox"}, .ImGui = {.Docking = false, .Viewports = false}});
    if (application)
    {
        application->PushLayer(Quasar::CreateScope<Sandbox::SandboxLayer>());
    }
    return application;
}
