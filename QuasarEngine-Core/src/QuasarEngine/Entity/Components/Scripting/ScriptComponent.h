#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Scripting/ScriptSystem.h>

#include <sol/sol.hpp>
#include <string>

namespace QuasarEngine
{
    class ScriptComponent : public Component 
	{
	public:
		ScriptComponent();
        ~ScriptComponent();

        void Initialize();
        void Destroy();

        std::string scriptPath = ""; //"Assets/Scripts/player.lua"

        sol::environment environment;

        sol::function startFunc;
        sol::function updateFunc;
        sol::function stopFunc;

        bool initialized = false;
    };
}