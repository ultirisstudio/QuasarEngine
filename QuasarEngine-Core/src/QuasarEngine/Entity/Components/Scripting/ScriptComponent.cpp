#include "qepch.h"
#include "ScriptComponent.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Scripting/ScriptSystem.h>

namespace QuasarEngine
{
    ScriptComponent::ScriptComponent()
    {
    }

    ScriptComponent::~ScriptComponent()
    {
        if (initialized)
            Destroy();
    }

    void ScriptComponent::Initialize()
    {
        Renderer::m_SceneData.m_ScriptSystem->RegisterEntityScript(*this);
    }

    void ScriptComponent::Destroy()
    {
        Renderer::m_SceneData.m_ScriptSystem->UnregisterEntityScript(*this);
    }
}