#include <EditorApplication.h>

#include <quasar/core/EntryPoint.h>

#include <string>
#include <utility>
#include <vector>

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    return Editor::CreateEditorApplication(std::move(arguments));
}
