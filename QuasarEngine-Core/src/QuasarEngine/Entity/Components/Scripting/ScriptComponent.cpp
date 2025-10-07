#include "qepch.h"
#include "ScriptComponent.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Scripting/ScriptSystem.h>

namespace QuasarEngine
{
    ScriptComponent::ScriptComponent() : reflectedVars()
    {
    }

    ScriptComponent::~ScriptComponent()
    {
        if (initialized)
            Destroy();
    }

    void ScriptComponent::Initialize()
    {
        Renderer::Instance().m_SceneData.m_ScriptSystem->RegisterEntityScript(*this);
    }

    void ScriptComponent::Destroy()
    {
        Renderer::Instance().m_SceneData.m_ScriptSystem->UnregisterEntityScript(*this);
    }
}