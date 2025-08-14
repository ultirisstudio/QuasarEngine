#include "qepch.h"
#include "ScriptSystem.h"

#include <iostream>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>

#include <QuasarEngine/Core/Input.h>
#include <glm/gtx/compatibility.hpp>

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

    sol::object ScriptSystem::LuaGetComponent(Entity& entity, const std::string& componentName, sol::state_view lua) {
        if (!entity) {
            std::cerr << "[Lua Error] Invalid Entity reference in getComponent\n";
            return sol::nil;
        }
        auto it = g_getComponentFuncs.find(componentName);
        if (it != g_getComponentFuncs.end()) return it->second(entity, lua);
        return sol::nil;
    }

    bool ScriptSystem::LuaHasComponent(Entity& entity, const std::string& componentName) {
        if (!entity) {
            std::cerr << "[Lua Error] Invalid Entity reference in hasComponent\n";
            return false;
        }
        auto it = g_hasComponentFuncs.find(componentName);
        return it != g_hasComponentFuncs.end() && it->second(entity);
    }

    sol::object ScriptSystem::LuaAddComponent(Entity& entity, const std::string& componentName, sol::variadic_args args, sol::state_view lua) {
        if (!entity) {
            std::cerr << "[Lua Error] Invalid Entity reference in addComponent\n";
            return sol::nil;
        }
        auto it = g_addComponentFuncs.find(componentName);
        if (it != g_addComponentFuncs.end()) return it->second(entity, args, lua);
        return sol::nil;
    }

    void ScriptSystem::Initialize()
    {
		// Register Lua functions for component management
        g_getComponentFuncs.insert({"TransformComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<TransformComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<TransformComponent>()))
                : sol::nil;
        } });
        g_getComponentFuncs.insert({ "MeshComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<MeshComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<MeshComponent>()))
                : sol::nil;
        } });

        g_getComponentFuncs.insert({ "MaterialComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<MaterialComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<MaterialComponent>()))
                : sol::nil;
		} });

        g_getComponentFuncs.insert({ "MeshRendererComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<MeshRendererComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<MeshRendererComponent>()))
                : sol::nil;
		} });

		// Register Lua functions for checking component existence

        g_hasComponentFuncs.insert({ "TransformComponent", [](Entity& e) { return e.HasComponent<TransformComponent>(); } });
        g_hasComponentFuncs.insert({ "MeshComponent", [](Entity& e) { return e.HasComponent<MeshComponent>(); } });
		g_hasComponentFuncs.insert({ "MaterialComponent", [](Entity& e) { return e.HasComponent<MaterialComponent>(); } });
		g_hasComponentFuncs.insert({ "MeshRendererComponent", [](Entity& e) { return e.HasComponent<MeshRendererComponent>(); } });

		// Register Lua functions for adding components

        g_addComponentFuncs.insert({ "TransformComponent", [](Entity& e, sol::variadic_args args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<TransformComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<TransformComponent>()));
            return sol::make_object(lua, std::ref(e.AddComponent<TransformComponent>()));
        } });
        g_addComponentFuncs.insert({ "MeshComponent", [](Entity& e, sol::variadic_args args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<MeshComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<MeshComponent>()));

            MeshComponent* comp = nullptr;
            if (args.size() >= 1 && args[0].is<std::string>()) {
                comp = &e.AddComponent<MeshComponent>(args[0].get<std::string>());
            }
            else {
                comp = &e.AddComponent<MeshComponent>();
            }
            return sol::make_object(lua, std::ref(*comp));
        } });

        g_addComponentFuncs.insert({ "MaterialComponent", [](Entity& e, sol::variadic_args args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<MaterialComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<MaterialComponent>()));
            MaterialComponent* comp = &e.AddComponent<MaterialComponent>();
            return sol::make_object(lua, std::ref(*comp));
		} });

        g_addComponentFuncs.insert({ "MeshRendererComponent", [](Entity& e, sol::variadic_args args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<MeshRendererComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<MeshRendererComponent>()));
            MeshRendererComponent* comp = &e.AddComponent<MeshRendererComponent>();
			return sol::make_object(lua, std::ref(*comp));
		} });

        BindInputToLua(lua);
        BindMathToLua(lua);
		BindFunctionToLua(lua);
		BindEntityToLua(lua);
    }

    void ScriptSystem::RegisterEntityScript(ScriptComponent& scriptComponent)
    {
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

    void ScriptSystem::BindInputToLua(sol::state& lua_state)
    {
        lua_state.set_function("is_key_pressed", [](int key) {
            return Input::IsKeyPressed(static_cast<KeyCode>(key));
            });

        lua_state.set_function("is_key_just_pressed", [](int key) {
            return Input::IsKeyJustPressed(static_cast<KeyCode>(key));
            });

        lua_state.set_function("is_mouse_button_pressed", [](int button) {
            return Input::IsMouseButtonPressed(static_cast<MouseCode>(button));
            });

        lua_state.set_function("get_mouse_position", []() {
            glm::vec2 pos = Input::GetMousePosition();
            return std::make_tuple(pos.x, pos.y);
            });

        lua_state.set_function("get_mouse_x", []() {
            return Input::GetMouseX();
            });

        lua_state.set_function("get_mouse_y", []() {
            return Input::GetMouseY();
            });

        lua_state["KEY_SPACE"] = static_cast<int>(Key::Space);
        lua_state["KEY_APOSTROPHE"] = static_cast<int>(Key::Apostrophe);
        lua_state["KEY_COMMA"] = static_cast<int>(Key::Comma);
        lua_state["KEY_MINUS"] = static_cast<int>(Key::Minus);
        lua_state["KEY_PERIOD"] = static_cast<int>(Key::Period);
        lua_state["KEY_SLASH"] = static_cast<int>(Key::Slash);
        lua_state["KEY_0"] = static_cast<int>(Key::D0);
        lua_state["KEY_1"] = static_cast<int>(Key::D1);
        lua_state["KEY_2"] = static_cast<int>(Key::D2);
        lua_state["KEY_3"] = static_cast<int>(Key::D3);
        lua_state["KEY_4"] = static_cast<int>(Key::D4);
        lua_state["KEY_5"] = static_cast<int>(Key::D5);
        lua_state["KEY_6"] = static_cast<int>(Key::D6);
        lua_state["KEY_7"] = static_cast<int>(Key::D7);
        lua_state["KEY_8"] = static_cast<int>(Key::D8);
        lua_state["KEY_9"] = static_cast<int>(Key::D9);
        lua_state["KEY_SEMICOLON"] = static_cast<int>(Key::Semicolon);
        lua_state["KEY_EQUAL"] = static_cast<int>(Key::Equal);
        lua_state["KEY_A"] = static_cast<int>(Key::A);
        lua_state["KEY_B"] = static_cast<int>(Key::B);
        lua_state["KEY_C"] = static_cast<int>(Key::C);
        lua_state["KEY_D"] = static_cast<int>(Key::D);
        lua_state["KEY_E"] = static_cast<int>(Key::E);
        lua_state["KEY_F"] = static_cast<int>(Key::F);
        lua_state["KEY_G"] = static_cast<int>(Key::G);
        lua_state["KEY_H"] = static_cast<int>(Key::H);
        lua_state["KEY_I"] = static_cast<int>(Key::I);
        lua_state["KEY_J"] = static_cast<int>(Key::J);
        lua_state["KEY_K"] = static_cast<int>(Key::K);
        lua_state["KEY_L"] = static_cast<int>(Key::L);
        lua_state["KEY_M"] = static_cast<int>(Key::M);
        lua_state["KEY_N"] = static_cast<int>(Key::N);
        lua_state["KEY_O"] = static_cast<int>(Key::O);
        lua_state["KEY_P"] = static_cast<int>(Key::P);
        lua_state["KEY_Q"] = static_cast<int>(Key::Q);
        lua_state["KEY_R"] = static_cast<int>(Key::R);
        lua_state["KEY_S"] = static_cast<int>(Key::S);
        lua_state["KEY_T"] = static_cast<int>(Key::T);
        lua_state["KEY_U"] = static_cast<int>(Key::U);
        lua_state["KEY_V"] = static_cast<int>(Key::V);
        lua_state["KEY_W"] = static_cast<int>(Key::W);
        lua_state["KEY_X"] = static_cast<int>(Key::X);
        lua_state["KEY_Y"] = static_cast<int>(Key::Y);
        lua_state["KEY_Z"] = static_cast<int>(Key::Z);
        lua_state["KEY_ESCAPE"] = static_cast<int>(Key::Escape);
        lua_state["KEY_ENTER"] = static_cast<int>(Key::Enter);
        lua_state["KEY_TAB"] = static_cast<int>(Key::Tab);
        lua_state["KEY_BACKSPACE"] = static_cast<int>(Key::Backspace);
        lua_state["KEY_INSERT"] = static_cast<int>(Key::Insert);
        lua_state["KEY_DELETE"] = static_cast<int>(Key::Delete);
        lua_state["KEY_RIGHT"] = static_cast<int>(Key::Right);
        lua_state["KEY_LEFT"] = static_cast<int>(Key::Left);
        lua_state["KEY_DOWN"] = static_cast<int>(Key::Down);
        lua_state["KEY_UP"] = static_cast<int>(Key::Up);
        lua_state["KEY_PAGE_UP"] = static_cast<int>(Key::PageUp);
        lua_state["KEY_PAGE_DOWN"] = static_cast<int>(Key::PageDown);
        lua_state["KEY_HOME"] = static_cast<int>(Key::Home);
        lua_state["KEY_END"] = static_cast<int>(Key::End);
        lua_state["KEY_CAPS_LOCK"] = static_cast<int>(Key::CapsLock);
        lua_state["KEY_SCROLL_LOCK"] = static_cast<int>(Key::ScrollLock);
        lua_state["KEY_NUM_LOCK"] = static_cast<int>(Key::NumLock);
        lua_state["KEY_PRINT_SCREEN"] = static_cast<int>(Key::PrintScreen);
        lua_state["KEY_PAUSE"] = static_cast<int>(Key::Pause);
        lua_state["KEY_F1"] = static_cast<int>(Key::F1);
        lua_state["KEY_F2"] = static_cast<int>(Key::F2);
        lua_state["KEY_F3"] = static_cast<int>(Key::F3);
        lua_state["KEY_F4"] = static_cast<int>(Key::F4);
        lua_state["KEY_F5"] = static_cast<int>(Key::F5);
        lua_state["KEY_F6"] = static_cast<int>(Key::F6);
        lua_state["KEY_F7"] = static_cast<int>(Key::F7);
        lua_state["KEY_F8"] = static_cast<int>(Key::F8);
        lua_state["KEY_F9"] = static_cast<int>(Key::F9);
        lua_state["KEY_F10"] = static_cast<int>(Key::F10);
        lua_state["KEY_F11"] = static_cast<int>(Key::F11);
        lua_state["KEY_F12"] = static_cast<int>(Key::F12);

        lua_state["MOUSE_BUTTON_LEFT"] = static_cast<int>(Mouse::ButtonLeft);
        lua_state["MOUSE_BUTTON_RIGHT"] = static_cast<int>(Mouse::ButtonRight);
        lua_state["MOUSE_BUTTON_MIDDLE"] = static_cast<int>(Mouse::ButtonMiddle);
    }

    void ScriptSystem::BindMathToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z
        );

        lua_state.new_usertype<glm::mat4>("mat4",
            sol::constructors<glm::mat4(), glm::mat4(float)>(),
            "identity", sol::var(glm::mat4(1.0f))
        );

        lua_state.new_usertype<glm::quat>("quat",
            sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
            "x", &glm::quat::x,
            "y", &glm::quat::y,
            "z", &glm::quat::z,
            "w", &glm::quat::w
        );

        lua_state.new_usertype<glm::vec2>("vec2",
            sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y
        );

        lua_state.new_usertype<glm::ivec2>("ivec2",
            sol::constructors<glm::ivec2(), glm::ivec2(int, int)>(),
            "x", &glm::ivec2::x,
            "y", &glm::ivec2::y
        );

        lua_state.new_usertype<glm::ivec3>("ivec3",
            sol::constructors<glm::ivec3(), glm::ivec3(int, int, int)>(),
            "x", &glm::ivec3::x,
            "y", &glm::ivec3::y,
            "z", &glm::ivec3::z
        );

        lua_state.new_usertype<glm::ivec4>("ivec4",
            sol::constructors<glm::ivec4(), glm::ivec4(int, int, int, int)>(),
            "x", &glm::ivec4::x,
            "y", &glm::ivec4::y,
            "z", &glm::ivec4::z,
            "w", &glm::ivec4::w
        );
    }

    void ScriptSystem::BindFunctionToLua(sol::state& lua_state)
    {
        lua_state.set_function("log", [](const std::string& message) {
            std::cout << "[Lua] " << message << std::endl;
            });

        lua_state.set_function("getTime", []() {
            return Renderer::GetTime();
            });

        lua_state.set_function("getScene", []() -> Scene* {
            return Renderer::m_SceneData.m_Scene;
            });

        lua_state.set_function("getEntityByName", [&lua_state](const std::string& name) -> sol::object {
            std::optional<Entity> ent = Renderer::m_SceneData.m_Scene->GetEntityByName(name);
            if (ent.has_value())
                return sol::make_object(lua_state, ent.value());
            return sol::nil;
            });

        lua_state.set_function("lookAtEuler", [](const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) -> glm::vec3 {
            glm::mat4 view = glm::lookAt(position, target, up);

            glm::quat q = glm::quat_cast(view);

            glm::vec3 euler = glm::degrees(glm::eulerAngles(q));

            return euler;
            });


        lua_state.set_function("lookAt", [](const glm::vec3& position,
            const glm::vec3& target,
            const glm::vec3& up) -> glm::mat4
            {
                glm::vec3 dir = target - position;

                if (glm::length(dir) < 1e-6f) {
                    return glm::mat4(1.0f);
                }
                dir = glm::normalize(dir);

                glm::vec3 upNorm = glm::normalize(up);

                if (glm::abs(glm::dot(upNorm, dir)) > 0.999f) {
                    upNorm = glm::vec3(0, 1, 0);
                    if (glm::abs(glm::dot(upNorm, dir)) > 0.999f) {
                        upNorm = glm::vec3(1, 0, 0);
                    }
                }

                return glm::lookAt(position, position + dir, upNorm);
            });

        lua_state.set_function("mat4_to_euler", [](const glm::mat4& mat) -> glm::vec3 {
            glm::mat4 rotMat = mat;
            
            for (int i = 0; i < 3; ++i) {
                glm::vec3 axis(rotMat[0][i], rotMat[1][i], rotMat[2][i]);
                if (glm::length(axis) > 1e-6f) {
                    axis = glm::normalize(axis);
                }
                rotMat[0][i] = axis.x;
                rotMat[1][i] = axis.y;
                rotMat[2][i] = axis.z;
            }

            glm::quat q = glm::quat_cast(rotMat);
            glm::vec3 euler = glm::eulerAngles(q); //glm::degrees(glm::eulerAngles(q));

            if (!glm::all(glm::isfinite(euler))) {
                return glm::vec3(0.0f);
            }

            return euler;
            });


        lua_state.set_function("inverse", [](const glm::mat4& mat) -> glm::mat4 {
            return glm::inverse(mat);
            });

        lua_state.set_function("radians_to_degrees", [](const glm::vec3& v) -> glm::vec3 {
            return glm::degrees(v);
            });
    }

    void ScriptSystem::BindEntityToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<Entity>("Entity");

        lua_state["Entity"]["getComponent"] = [this, &lua_state](Entity& entity, const std::string& componentName) -> sol::object {
            return LuaGetComponent(entity, componentName, lua_state);
            };

		lua_state["Entity"]["hasComponent"] = [this](Entity& entity, const std::string& componentName) -> bool {
            return LuaHasComponent(entity, componentName);
			};

        lua_state["Entity"]["addComponent"] = [this, &lua_state](Entity& entity, const std::string& componentName, sol::variadic_args args) -> sol::object {
            return LuaAddComponent(entity, componentName, args, lua_state);
            };

        lua_state.new_usertype<TransformComponent>("Transform",
            "position", &TransformComponent::Position,
            "rotation", &TransformComponent::Rotation,
            "scale", &TransformComponent::Scale
        );

        lua_state.new_usertype<MeshComponent>("Mesh",
            sol::constructors<MeshComponent(), MeshComponent(const std::string&)>(),
            "getName", &MeshComponent::GetName,
            "hasMesh", &MeshComponent::HasMesh,
            "generateMesh", [](MeshComponent& meshComp, sol::table vertices, sol::table indices) {
                std::vector<float> verts;
                std::vector<unsigned int> inds;

                for (auto& pair : vertices) {
                    verts.push_back(pair.second.as<float>());
                }

                for (auto& pair : indices) {
                    inds.push_back(pair.second.as<unsigned int>());
                }

                meshComp.GenerateMesh(verts, inds);
            }
        );
	}
}