#include "SandboxLayer.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/EntryPoint.h>

#include <glm/vec4.hpp>

#include <string>
#include <utility>
#include <vector>

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    Quasar::Scope<Application> application = Application::Create({.Name = "Sandbox",
                                                                  .Arguments = std::move(arguments),
                                                                  .Window = {.Title = "Sandbox"},
                                                                  .ImGui = {.Docking = false, .Viewports = false},
                                                                  .ClearColor = glm::vec4(0.45f, 0.62f, 0.85f, 1.0f)});
    if (application)
    {
        application->PushLayer(Quasar::CreateScope<Sandbox::SandboxLayer>());
    }
    return application;
}
