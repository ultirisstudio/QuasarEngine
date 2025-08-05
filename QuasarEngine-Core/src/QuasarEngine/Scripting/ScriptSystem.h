#pragma once

#include <entt.hpp>
#include <sol/sol.hpp>

namespace QuasarEngine
{
    class ScriptComponent;

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
        std::unique_ptr<entt::registry> m_Registry;

        sol::state lua;
    };
}

