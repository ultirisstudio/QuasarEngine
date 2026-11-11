#include "EditorLayer.h"

#include <quasar/log/Log.h>

namespace Quasar::Editor
{
    EditorLayer::EditorLayer() : Layer("Editor"), m_Console(Log::GetMemorySink()) {}

    void EditorLayer::OnAttach()
    {
        QUASAR_INFO("Editor started");
    }

    void EditorLayer::OnDetach()
    {
        QUASAR_INFO("Editor stopped");
    }

    void EditorLayer::OnImGuiRender()
    {
        m_Console.OnImGuiRender();
    }
}
