#include "EditorApplication.h"
#include "EditorLayer.h"

#include <utility>

namespace Quasar::Editor
{
    ApplicationSpecification GetEditorSpecification(std::vector<std::string> arguments)
    {
        return {.Name = "Quasar Editor", .Arguments = std::move(arguments), .Window = {.Title = "Quasar Editor", .Width = 1600, .Height = 900}};
    }

    Scope<Application> CreateEditorApplication(std::vector<std::string> arguments)
    {
        Scope<Application> application = Application::Create(GetEditorSpecification(std::move(arguments)));
        if (application)
        {
            application->PushLayer(CreateScope<EditorLayer>());
        }

        return application;
    }
}
