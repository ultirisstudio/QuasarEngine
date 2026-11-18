#pragma once

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>

#include <string>
#include <vector>

namespace Quasar::Editor
{
    ApplicationSpecification GetEditorSpecification(std::vector<std::string> arguments);

    [[nodiscard]] Scope<Application> CreateEditorApplication(std::vector<std::string> arguments);
}
