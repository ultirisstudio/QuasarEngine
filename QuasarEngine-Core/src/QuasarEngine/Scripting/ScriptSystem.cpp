#include "qepch.h"
#include "ScriptSystem.h"

#include <iostream>

#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Scripting/ScriptComponent.h>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>

namespace QuasarEngine
{
    ScriptSystem::ScriptSystem() {
        m_Registry = std::make_unique<entt::registry>();

        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    }

    ScriptSystem::~ScriptSystem()
    {
		auto view = m_Registry->view<ScriptComponent>();
        for (auto entity : view) {
			auto& scriptComponent = view.get<ScriptComponent>(entity);
            if (scriptComponent.initialized) {
				UnregisterEntityScript(scriptComponent);
			}
		}

		m_Registry.reset();
    }

    sol::object LuaGetComponent(Entity& entity, const std::string& componentName, sol::state_view lua);
    sol::object LuaGetComponent(Entity& entity, const std::string& componentName, sol::state_view lua) {
        if (!entity) {
            std::cerr << "[Lua Error] Invalid Entity reference in getComponent\n";
            return sol::nil;
        }

        if (componentName == "TransformComponent") {
            if (!entity.HasComponent<TransformComponent>()) {
                return sol::nil;
            }
            TransformComponent& comp = entity.GetComponent<TransformComponent>();
            return sol::make_object(lua, std::ref(comp));
        }

        return sol::nil;
    }


    void ScriptSystem::Initialize() {
        lua.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z
        );

        lua.new_usertype<Entity>("Entity");

        lua["Entity"]["getComponent"] = [this](Entity& entity, const std::string& componentName) -> sol::object {
            return LuaGetComponent(entity, componentName, lua);
        };

        lua.new_usertype<TransformComponent>("Transform",
            "position", &TransformComponent::Position,
            "rotation", &TransformComponent::Rotation,
            "scale", &TransformComponent::Scale
        );
    }

    void ScriptSystem::RegisterEntityScript(ScriptComponent& scriptComponent) {
        entt::entity entity = scriptComponent.entt_entity;

        if (!m_Registry->all_of<ScriptComponent>(entity)) {
            m_Registry->emplace<ScriptComponent>(entity, scriptComponent);
        }

        sol::environment env(lua, sol::create, lua.globals());

        sol::table publicTable = lua.create_table();
        env["public"] = publicTable;

        Entity entityObject{ entity, Renderer::m_SceneData.m_Scene->GetRegistry() };
        env["entity"] = entityObject;
        env["self"] = entityObject;

        env.set_function("vec3", [](float x, float y, float z) {
            return glm::vec3(x, y, z);
            });

        try {
            lua.script_file(scriptComponent.scriptPath, env);

            sol::object maybePublic = env["public"];
            if (maybePublic.is<sol::table>()) {
                publicTable = maybePublic.as<sol::table>();
            }

            scriptComponent.publicTable = publicTable;
            scriptComponent.reflectedVars.clear();

            for (auto&& kv : publicTable) {
                std::string key = kv.first.as<std::string>();
                sol::object value = kv.second;

                ScriptComponent::ReflectedVar rv;
                rv.name = key;

                switch (value.get_type()) {
                case sol::type::number:   rv.type = ScriptComponent::VarType::Number; break;
                case sol::type::string:   rv.type = ScriptComponent::VarType::String; break;
                case sol::type::boolean:  rv.type = ScriptComponent::VarType::Boolean; break;
                case sol::type::userdata:
                case sol::type::table: {
                    if (value.is<glm::vec3>()) rv.type = ScriptComponent::VarType::Vec3;
                    else continue;
                    break;
                }
                default: continue;
                }

                scriptComponent.reflectedVars.push_back(std::move(rv));
            }

            scriptComponent.startFunc = env["start"];
            scriptComponent.updateFunc = env["update"];
            scriptComponent.stopFunc = env["stop"];

            scriptComponent.environment = std::move(env);
            scriptComponent.initialized = true;

            /*if (scriptComponent.publicTable.valid()) {
                for (auto&& kv : scriptComponent.publicTable) {
                    std::string key = kv.first.as<std::string>();
                    preserved.emplace(key, kv.second);
                }
            }

            if (scriptComponent.publicTable.valid()) {
                for (auto& [key, obj] : preserved) {
                    if (scriptComponent.publicTable[key].valid())
                        scriptComponent.publicTable[key] = obj;
                    scriptComponent.environment[key] = scriptComponent.publicTable[key];
                }
            }*/
        }
        catch (const sol::error& e) {
            std::cerr << "[Lua Error] " << e.what() << std::endl;
        }
    }

    void ScriptSystem::UnregisterEntityScript(ScriptComponent& scriptComponent)
    {
        auto view = m_Registry->view<ScriptComponent>();

        for (auto entity : view) {
            if (view.get<ScriptComponent>(entity).entt_entity == scriptComponent.entt_entity) {
				m_Registry->remove<sol::environment>(entity);
				break;
			}
		}

		scriptComponent.initialized = false;
		scriptComponent.startFunc = sol::nil;
		scriptComponent.updateFunc = sol::nil;
		scriptComponent.stopFunc = sol::nil;
		scriptComponent.environment = sol::nil;
    }

    void ScriptSystem::Update(float dt) {
        auto view = m_Registry->view<ScriptComponent>();
        
        for (auto e : view) {
            Entity entity{ e, Renderer::m_SceneData.m_Scene->GetRegistry() };
            auto& script = entity.GetComponent<ScriptComponent>();
            
            if (script.updateFunc.valid()) {
                try {
                    script.updateFunc(dt);
                }
                catch (const sol::error& e) {
                    std::cerr << "[Lua Runtime Error] " << e.what() << std::endl;
                }
            }
        }
    }

    void ScriptSystem::Start()
    {
        auto view = m_Registry->view<ScriptComponent>();

        for (auto e : view) {
            Entity entity{ e, Renderer::m_SceneData.m_Scene->GetRegistry() };
            auto& script = entity.GetComponent<ScriptComponent>();

            if (script.startFunc.valid()) {
                script.startFunc();
            }
        }
    }

    void ScriptSystem::Stop()
    {
        auto view = m_Registry->view<ScriptComponent>();

        for (auto e : view) {
            Entity entity{ e, Renderer::m_SceneData.m_Scene->GetRegistry() };
            auto& script = entity.GetComponent<ScriptComponent>();

            if (script.stopFunc.valid()) {
                script.stopFunc();
            }
        }
    }
}