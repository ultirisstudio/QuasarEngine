#include "EditorLayer.h"

#include <quasar/core/Log.h>

namespace Quasar::Editor
{
    EditorLayer::EditorLayer() : Layer("Editor") {}

    void EditorLayer::OnAttach()
    {
        QUASAR_INFO("Editor started");
    }

    void EditorLayer::OnDetach()
    {
        QUASAR_INFO("Editor stopped");
    }
}
