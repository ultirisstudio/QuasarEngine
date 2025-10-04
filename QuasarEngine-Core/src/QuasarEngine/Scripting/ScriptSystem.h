#pragma once

#include <entt.hpp>
#include <sol/sol.hpp>

namespace QuasarEngine
{
    class ScriptComponent;
	class Entity;

    class ScriptSystem
    {
    public:
        ScriptSystem();
        ~ScriptSystem();

        void Initialize();
        void Update(float dt);

        void Start();
        void Stop();

        void RegisterEntityScript(ScriptComponent& scriptComponent);
        void UnregisterEntityScript(ScriptComponent& scriptComponent);

    private:
        using GetFunc = std::function<sol::object(Entity&, sol::state_view)>;
        using HasFunc = std::function<bool(Entity&)>;
        using AddFunc = std::function<sol::object(Entity&, sol::variadic_args, sol::state_view)>;

        std::unordered_map<std::string, GetFunc> g_getComponentFuncs;
        std::unordered_map<std::string, HasFunc> g_hasComponentFuncs;
        std::unordered_map<std::string, AddFunc> g_addComponentFuncs;

        sol::object LuaGetComponent(Entity& entity, const std::string& componentName, sol::state_view lua);
        bool LuaHasComponent(Entity& entity, const std::string& componentName);
        sol::object LuaAddComponent(Entity& entity, const std::string& componentName, sol::variadic_args args, sol::state_view lua);

        void BindInputToLua(sol::state& lua_state);
        void BindMathToLua(sol::state& lua_state);
        void BindFunctionToLua(sol::state& lua_state);
        void BindEntityToLua(sol::state& lua_state);
        void BindPhysicsToLua(sol::state& lua_state);

        std::unique_ptr<entt::registry> m_Registry;

        sol::state m_Lua;
    };
}

