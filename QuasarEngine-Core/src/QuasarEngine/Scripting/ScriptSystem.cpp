#include "qepch.h"
#include "ScriptSystem.h"

#include <iostream>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>

#include <QuasarEngine/Core/Input.h>
#include <glm/gtx/compatibility.hpp>

#include <QuasarEngine/Physic/PhysicEngine.h>

namespace QuasarEngine
{
    namespace {
        inline sol::object ActorToEntityObject(physx::PxActor* a, sol::state_view lua) {
            if (!a || !a->userData) return sol::nil;
            const auto id = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(a->userData));
            auto* reg = QuasarEngine::Renderer::m_SceneData.m_Scene->GetRegistry();
            QuasarEngine::Entity e{ id, reg };
            if (!e.IsValid()) return sol::nil;
            return sol::make_object(lua, e);
        }
    }

    ScriptSystem::ScriptSystem() {
        m_Registry = std::make_unique<entt::registry>();

        m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::os);
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
        g_getComponentFuncs.insert({ "TagComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<TagComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<TagComponent>()))
                : sol::nil;
        } });

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

        g_getComponentFuncs.insert({ "RigidBodyComponent", [](Entity& e, sol::state_view lua) -> sol::object {
            return e.HasComponent<RigidBodyComponent>()
                ? sol::make_object(lua, std::ref(e.GetComponent<RigidBodyComponent>()))
                : sol::nil;
        } });

        g_getComponentFuncs.insert({ "BoxColliderComponent", [](Entity& e, sol::state_view lua)->sol::object {
            return e.HasComponent<BoxColliderComponent>() ? sol::make_object(lua, std::ref(e.GetComponent<BoxColliderComponent>())) : sol::nil;
        } });

        g_getComponentFuncs.insert({ "ScriptComponent", [](Entity& e, sol::state_view lua)->sol::object {
            return e.HasComponent<ScriptComponent>() ? sol::make_object(lua, std::ref(e.GetComponent<ScriptComponent>())) : sol::nil;
        } });

        g_hasComponentFuncs.insert({ "TagComponent", [](Entity& e) { return e.HasComponent<TagComponent>(); } });
        g_hasComponentFuncs.insert({ "TransformComponent", [](Entity& e) { return e.HasComponent<TransformComponent>(); } });
        g_hasComponentFuncs.insert({ "MeshComponent", [](Entity& e) { return e.HasComponent<MeshComponent>(); } });
		g_hasComponentFuncs.insert({ "MaterialComponent", [](Entity& e) { return e.HasComponent<MaterialComponent>(); } });
		g_hasComponentFuncs.insert({ "MeshRendererComponent", [](Entity& e) { return e.HasComponent<MeshRendererComponent>(); } });
        g_hasComponentFuncs.insert({ "RigidBodyComponent", [](Entity& e) { return e.HasComponent<RigidBodyComponent>(); } });
        g_hasComponentFuncs.insert({ "BoxColliderComponent", [](Entity& e) { return e.HasComponent<BoxColliderComponent>(); } });
        g_hasComponentFuncs.insert({ "ScriptComponent", [](Entity& e) { return e.HasComponent<ScriptComponent>(); } });

        g_addComponentFuncs.insert({ "TagComponent", [](Entity& e, sol::variadic_args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<TagComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<TagComponent>()));
            auto& c = e.AddComponent<TagComponent>();
            return sol::make_object(lua, std::ref(c));
        } });

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
        
        g_addComponentFuncs.insert({ "RigidBodyComponent", [](Entity& e, sol::variadic_args, sol::state_view lua) -> sol::object {
            if (e.HasComponent<RigidBodyComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<RigidBodyComponent>()));
            auto& c = e.AddComponent<RigidBodyComponent>();
            c.Init();
            return sol::make_object(lua, std::ref(c));
        } });

        g_addComponentFuncs.insert({ "BoxColliderComponent", [](Entity& e, sol::variadic_args, sol::state_view lua)->sol::object {
            auto& c = e.HasComponent<BoxColliderComponent>() ? e.GetComponent<BoxColliderComponent>()
                                                             : e.AddComponent<BoxColliderComponent>();
            c.Init();
            return sol::make_object(lua, std::ref(c));
        } });

        g_addComponentFuncs.insert({ "ScriptComponent", [](Entity& e, sol::variadic_args, sol::state_view lua)->sol::object {
            auto& c = e.HasComponent<ScriptComponent>() ? e.GetComponent<ScriptComponent>() : e.AddComponent<ScriptComponent>();
            return sol::make_object(lua, std::ref(c));
        } });

        BindInputToLua(m_Lua);
        BindMathToLua(m_Lua);
		BindFunctionToLua(m_Lua);
		BindEntityToLua(m_Lua);
        BindPhysicsToLua(m_Lua);
    }

    void ScriptSystem::RegisterEntityScript(ScriptComponent& scriptComponent)
    {
        entt::entity entity = scriptComponent.entt_entity;

        if (!m_Registry->all_of<ScriptComponent>(entity)) {
            m_Registry->emplace<ScriptComponent>(entity, scriptComponent);
        }

        sol::environment env(m_Lua, sol::create, m_Lua.globals());

        sol::table publicTable = m_Lua.create_table();
        env["public"] = publicTable;

        Entity entityObject{ entity, Renderer::m_SceneData.m_Scene->GetRegistry() };
        env["entity"] = entityObject;
        env["self"] = entityObject;

        env.set_function("vec3", [](float x, float y, float z) {
            return glm::vec3(x, y, z);
            });

        try {
            m_Lua.script_file(scriptComponent.scriptPath, env);

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
            "z", &glm::vec3::z,
            sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; }
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

        lua_state.set_function("deg2rad", [](float deg) {
			return glm::radians(deg);
            });

        lua_state.set_function("clamp", [](float x, float a, float b) {
			return glm::clamp(x, a, b);
            });

        lua_state.set_function("normalize", [](glm::vec3 v) {
			return glm::normalize(v);
            });

        lua_state.set_function("forward_from_yawpitch", [](float yawDeg, float pitchDeg) {
            float yaw = glm::radians(yawDeg);
            float pitch = glm::radians(pitchDeg);
            float cp = cos(pitch);
            return glm::vec3(cp * sin(yaw), sin(pitch), -cp * cos(yaw));
            });

        lua_state.set_function("right_from_yaw", [](float yawDeg) {
            float yaw = glm::radians(yawDeg);
            return glm::vec3(cos(yaw), 0, sin(yaw));
	        });

        lua_state.set_function("lookAtEuler", [](const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) -> glm::vec3 {
            glm::mat4 view = glm::lookAt(position, target, up);

            glm::quat q = glm::quat_cast(view);

            glm::vec3 euler = glm::degrees(glm::eulerAngles(q));

            return euler;
            });

        lua_state.set_function("lookAt", [](const glm::vec3& position,
            const glm::vec3& target,
            const glm::vec3& up) -> glm::mat4 {
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

    void ScriptSystem::BindFunctionToLua(sol::state& lua_state)
    {
        lua_state.set_function("createEntity", [](const std::string& name) -> Entity {
            auto* scene = Renderer::m_SceneData.m_Scene;
            if (!scene) return {};
            return scene->CreateEntity(name);
            });

        lua_state.set_function("setParent", [](Entity& child, Entity& parent) {
            if (!child.IsValid() || !parent.IsValid()) return;
            if (child.HasComponent<HierarchyComponent>())
                child.GetComponent<HierarchyComponent>().m_Parent = parent.GetUUID();
            });


        lua_state.set_function("log", [](const std::string& message) {
            std::cout << "[Lua] " << message << std::endl;
            });

        lua_state.set_function("getTime", []() {
            return Renderer::GetTime();
            });

        lua_state.set_function("getScene", []() -> Scene* {
            return Renderer::m_SceneData.m_Scene;
            });

        lua_state.set_function("destroyEntity", [](Entity& e) {
            if (e.IsValid()) Renderer::m_SceneData.m_Scene->DestroyEntity(e);
            });

        lua_state.set_function("removeEntityByName", [&lua_state](const std::string& name) -> sol::object {
            std::optional<Entity> ent = Renderer::m_SceneData.m_Scene->GetEntityByName(name);
            if (ent.has_value())
                Renderer::m_SceneData.m_Scene->DestroyEntity(ent.value());
            return sol::nil;
            });

        lua_state.set_function("getEntityByName", [&lua_state](const std::string& name) -> sol::object {
            std::optional<Entity> ent = Renderer::m_SceneData.m_Scene->GetEntityByName(name);
            if (ent.has_value())
                return sol::make_object(lua_state, ent.value());
            return sol::nil;
            });

        lua_state.set_function("attachScript", [this](Entity& e, const std::string& path) {
            if (!e.IsValid()) return;
            auto& sc = e.HasComponent<ScriptComponent>() ? e.GetComponent<ScriptComponent>()
                : e.AddComponent<ScriptComponent>();
            sc.scriptPath = path;

            sc.Initialize();

            if (sc.startFunc.valid()) {
                sc.startFunc();
            }
            });
    }

    void ScriptSystem::BindEntityToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<Entity>("Entity",
            "name", &Entity::GetName,
            "id", & Entity::GetUUID
        );

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

        lua_state.new_enum<QuasarEngine::TagMask>("TagMask", {
            {"None",        QuasarEngine::TagMask::None},
            {"Player",      QuasarEngine::TagMask::Player},
            {"Enemy",       QuasarEngine::TagMask::Enemy},
            {"NPC",         QuasarEngine::TagMask::NPC},
            {"Collectible", QuasarEngine::TagMask::Collectible},
            {"Trigger",     QuasarEngine::TagMask::Trigger},
            {"Static",      QuasarEngine::TagMask::Static},
            {"Dynamic",     QuasarEngine::TagMask::Dynamic},
            {"Boss",        QuasarEngine::TagMask::Boss},
            {"Projectile",  QuasarEngine::TagMask::Projectile},
            {"All",         QuasarEngine::TagMask::All}
        });

        lua_state.new_usertype<QuasarEngine::TagComponent>("TagComponent",
            "Tag", &QuasarEngine::TagComponent::Tag,
            "Add", &QuasarEngine::TagComponent::Add,
            "Remove", &QuasarEngine::TagComponent::Remove,
            "Set", &QuasarEngine::TagComponent::Set,
            "HasAny", &QuasarEngine::TagComponent::HasAny,
            "HasAll", &QuasarEngine::TagComponent::HasAll,
            "IsEnemy", &QuasarEngine::TagComponent::IsEnemy,
            "IsPlayer", &QuasarEngine::TagComponent::IsPlayer
        );

        lua_state.new_usertype<BoxColliderComponent>("BoxCollider",
            "mass", &BoxColliderComponent::mass,
            "friction", &BoxColliderComponent::friction,
            "bounciness", &BoxColliderComponent::bounciness,
            "useEntityScale", &BoxColliderComponent::m_UseEntityScale,
            "size", &BoxColliderComponent::m_Size,
            "Init", &BoxColliderComponent::Init
        );

	}

    void ScriptSystem::BindPhysicsToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<RigidBodyComponent>("RigidBody",
            "enableGravity", sol::property(
                [](RigidBodyComponent& c) { return c.enableGravity; },
                [](RigidBodyComponent& c, bool v) { c.enableGravity = v; c.UpdateEnableGravity(); }
            ),
            "set_body_type", [](RigidBodyComponent& c, const std::string& t) { c.bodyTypeString = t; c.UpdateBodyType(); },
            "set_linear_damping", [](RigidBodyComponent& c, float v) { c.linearDamping = v; c.UpdateDamping(); },
            "set_angular_damping", [](RigidBodyComponent& c, float v) { c.angularDamping = v; c.UpdateDamping(); }
        );

        sol::table physics = lua_state.create_table();

        physics.set_function("get_gravity", []() -> glm::vec3 {
            auto* scene = PhysicEngine::Instance().GetScene();
            if (!scene) return { 0.f, -9.81f, 0.f };
            return ToGlm(scene->getGravity());
            });

        physics.set_function("set_gravity", [](const glm::vec3& g) {
            PhysicEngine::Instance().SetGravity(ToPx(g));
            });

        physics.set_function("raycast",
            [&lua_state](const glm::vec3& origin,
                const glm::vec3& dir,
                float maxDist,
                sol::optional<bool> queryDynamic,
                sol::optional<bool> queryStatic) -> sol::object
            {
                auto* scene = PhysicEngine::Instance().GetScene();
                if (!scene) return sol::nil;

                physx::PxQueryFilterData f{};
                f.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;
                if (queryDynamic && !queryDynamic.value()) f.flags &= ~physx::PxQueryFlag::eDYNAMIC;
                if (queryStatic && !queryStatic.value())  f.flags &= ~physx::PxQueryFlag::eSTATIC;

                physx::PxRaycastBuffer hit;
                const bool ok = scene->raycast(ToPx(origin), ToPx(glm::normalize(dir)), maxDist,
                    hit, physx::PxHitFlag::eDEFAULT, f);

                sol::table res = lua_state.create_table();
                res["hit"] = ok && hit.hasBlock;
                if (ok && hit.hasBlock) {
                    res["position"] = ToGlm(hit.block.position);
                    res["normal"] = ToGlm(hit.block.normal);
                    res["distance"] = hit.block.distance;
                    res["entity"] = ActorToEntityObject(hit.block.actor, lua_state);
                    res["shape"] = (uintptr_t)hit.block.shape;
                }
                return sol::make_object(lua_state, res);
            });

        physics.set_function("overlap_sphere",
            [&lua_state](const glm::vec3& center, float radius) -> sol::object
            {
                auto* scene = PhysicEngine::Instance().GetScene();
                if (!scene) return sol::nil;

                physx::PxSphereGeometry geo(radius);
                physx::PxTransform pose(ToPx(center));
                physx::PxOverlapBuffer buf;

                if (!scene->overlap(geo, pose, buf)) {
                    sol::table t = lua_state.create_table();
                    t["entities"] = lua_state.create_table();
                    t["count"] = 0;
                    return sol::make_object(lua_state, t);
                }

                sol::table ents = lua_state.create_table();
                uint32_t idx = 1;
                const physx::PxU32 n = buf.getNbTouches();
                const physx::PxOverlapHit* touches = buf.getTouches();
                for (physx::PxU32 i = 0; i < n; ++i) {
                    physx::PxActor* a = touches[i].actor;
                    auto obj = ActorToEntityObject(a, lua_state);
                    if (obj != sol::nil) ents[idx++] = obj;
                }

                sol::table t = lua_state.create_table();
                t["entities"] = ents;
                t["count"] = idx - 1;
                return sol::make_object(lua_state, t);
            });

        physics.set_function("sweep_sphere",
            [&lua_state](const glm::vec3& origin, float radius, const glm::vec3& dir, float maxDist) -> sol::object
            {
                auto* scene = PhysicEngine::Instance().GetScene();
                if (!scene) return sol::nil;

                physx::PxSphereGeometry geo(radius);
                physx::PxTransform pose(ToPx(origin));
                const physx::PxVec3 direction = ToPx(glm::normalize(dir));
                physx::PxSweepBuffer buf;

                const bool ok = scene->sweep(geo, pose, direction, maxDist, buf, physx::PxHitFlag::eDEFAULT);

                sol::table res = lua_state.create_table();
                res["hit"] = ok && buf.hasBlock;
                if (ok && buf.hasBlock) {
                    res["position"] = ToGlm(buf.block.position);
                    res["normal"] = ToGlm(buf.block.normal);
                    res["distance"] = buf.block.distance;
                    res["entity"] = ActorToEntityObject(buf.block.actor, lua_state);
                }
                return sol::make_object(lua_state, res);
            });

        physics.set_function("set_linear_velocity",
            [](Entity& e, const glm::vec3& v)
            {
                if (!e.HasComponent<RigidBodyComponent>()) return;
                auto& rbc = e.GetComponent<RigidBodyComponent>();
                if (physx::PxRigidDynamic* dyn = rbc.GetDynamic()) {
                    dyn->setLinearVelocity(ToPx(v));
                }
            });

        physics.set_function("add_force",
            [](Entity& e, const glm::vec3& f, sol::optional<std::string> modeStr)
            {
                if (!e.HasComponent<RigidBodyComponent>()) return;
                auto& rbc = e.GetComponent<RigidBodyComponent>();
                if (physx::PxRigidDynamic* dyn = rbc.GetDynamic()) {
                    physx::PxForceMode::Enum mode = physx::PxForceMode::eFORCE;
                    if (modeStr && (*modeStr == "impulse" || *modeStr == "IMPULSE"))
                        mode = physx::PxForceMode::eIMPULSE;
                    dyn->addForce(ToPx(f), mode, true);
                }
            });

        lua_state["physics"] = physics;
    }
}