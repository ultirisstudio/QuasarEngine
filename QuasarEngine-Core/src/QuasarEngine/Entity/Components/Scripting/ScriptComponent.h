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

        std::string scriptPath = "";

        sol::environment environment;

        sol::function startFunc;
        sol::function updateFunc;
        sol::function stopFunc;

        sol::table publicTable;

        enum class VarType { Number, String, Boolean, Vec3 };

        struct ReflectedVar {
            std::string name;
            VarType type;
        };

        std::vector<ReflectedVar> reflectedVars;

        bool initialized = false;
    };
}