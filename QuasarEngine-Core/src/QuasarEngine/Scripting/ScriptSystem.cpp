#include "qepch.h"
#include "ScriptSystem.h"

#include <iostream>

#include <QuasarEngine/Entity/Components/Scripting/ScriptComponent.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Entity/Components/Physics/BoxColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/SphereColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/CapsuleColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/CharacterControllerComponent.h>
#include <QuasarEngine/Entity/Components/Animation/AnimationComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Entity/Entity.h>

#include <QuasarEngine/Core/Input.h>
#include <glm/gtx/compatibility.hpp>

#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Physic/PhysXQueryUtils.h>

#include <QuasarEngine/UI/UIElement.h>
#include <QuasarEngine/UI/UIButton.h>
#include <QuasarEngine/UI/UICheckbox.h>
#include <QuasarEngine/UI/UITextInput.h>
#include <QuasarEngine/UI/UIProgressBar.h>
#include <QuasarEngine/UI/UIContainer.h>
#include <QuasarEngine/UI/UISlider.h>
#include <QuasarEngine/UI/UIText.h>
#include <QuasarEngine/UI/UISystem.h>

namespace QuasarEngine
{
    namespace {
        template <typename C>
        ScriptSystem::GetFunc MakeGet() {
            return [](Entity& e, sol::state_view lua) -> sol::object {
                return e.HasComponent<C>() ? sol::make_object(lua, std::ref(e.GetComponent<C>())) : sol::nil;
                };
        }

        template <typename C>
        ScriptSystem::HasFunc MakeHas() {
            return [](Entity& e) { return e.HasComponent<C>(); };
        }

        template <typename C>
        ScriptSystem::AddFunc MakeAddSimple() {
            return [](Entity& e, sol::variadic_args, sol::state_view lua) -> sol::object {
                if (e.HasComponent<C>()) return sol::make_object(lua, std::ref(e.GetComponent<C>()));
                auto& c = e.AddComponent<C>();
                return sol::make_object(lua, std::ref(c));
                };
        }

        template <typename C, typename Arg0>
        ScriptSystem::AddFunc MakeAddOptionalArg() {
            return [](Entity& e, sol::variadic_args args, sol::state_view lua) -> sol::object {
                if (e.HasComponent<C>()) return sol::make_object(lua, std::ref(e.GetComponent<C>()));
                C* comp = nullptr;
                if (args.size() >= 1 && args[0].is<Arg0>()) comp = &e.AddComponent<C>(args[0].get<Arg0>());
                else                                        comp = &e.AddComponent<C>();
                return sol::make_object(lua, std::ref(*comp));
                };
        }
    }

    ScriptSystem::ScriptSystem() {
        m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::os);
    }

    ScriptSystem::~ScriptSystem()
    {
		
    }

    sol::object ScriptSystem::LuaGetComponent(Entity& entity, const std::string& componentName, sol::state_view lua) {
        if (!entity.IsValid()) {
            std::cerr << "[Lua Error] Invalid Entity reference in getComponent\n";
            return sol::nil;
        }
        auto it = g_getComponentFuncs.find(componentName);
        if (it != g_getComponentFuncs.end()) return it->second(entity, lua);
        return sol::nil;
    }

    bool ScriptSystem::LuaHasComponent(Entity& entity, const std::string& componentName) {
        if (!entity.IsValid()) {
            std::cerr << "[Lua Error] Invalid Entity reference in hasComponent\n";
            return false;
        }
        auto it = g_hasComponentFuncs.find(componentName);
        return it != g_hasComponentFuncs.end() && it->second(entity);
    }

    sol::object ScriptSystem::LuaAddComponent(Entity& entity, const std::string& componentName, sol::variadic_args args, sol::state_view lua) {
        if (!entity.IsValid()) {
            std::cerr << "[Lua Error] Invalid Entity reference in addComponent\n";
            return sol::nil;
        }
        auto it = g_addComponentFuncs.find(componentName);
        if (it != g_addComponentFuncs.end()) return it->second(entity, args, lua);
        return sol::nil;
    }

    void ScriptSystem::Initialize()
    {
        g_getComponentFuncs["TagComponent"] = MakeGet<TagComponent>();
        g_hasComponentFuncs["TagComponent"] = MakeHas<TagComponent>();
        g_addComponentFuncs["TagComponent"] = MakeAddSimple<TagComponent>();

        g_getComponentFuncs["TransformComponent"] = MakeGet<TransformComponent>();
        g_hasComponentFuncs["TransformComponent"] = MakeHas<TransformComponent>();
        g_addComponentFuncs["TransformComponent"] = MakeAddSimple<TransformComponent>();

        g_getComponentFuncs["MeshComponent"] = MakeGet<MeshComponent>();
        g_hasComponentFuncs["MeshComponent"] = MakeHas<MeshComponent>();
        g_addComponentFuncs["MeshComponent"] = MakeAddOptionalArg<MeshComponent, std::string>();

        g_getComponentFuncs["MaterialComponent"] = MakeGet<MaterialComponent>();
        g_hasComponentFuncs["MaterialComponent"] = MakeHas<MaterialComponent>();
        g_addComponentFuncs["MaterialComponent"] = MakeAddSimple<MaterialComponent>();

        g_getComponentFuncs["MeshRendererComponent"] = MakeGet<MeshRendererComponent>();
        g_hasComponentFuncs["MeshRendererComponent"] = MakeHas<MeshRendererComponent>();
        g_addComponentFuncs["MeshRendererComponent"] = MakeAddSimple<MeshRendererComponent>();

        g_getComponentFuncs["RigidBodyComponent"] = MakeGet<RigidBodyComponent>();
        g_hasComponentFuncs["RigidBodyComponent"] = MakeHas<RigidBodyComponent>();
        g_addComponentFuncs["RigidBodyComponent"] = [](Entity& e, sol::variadic_args, sol::state_view lua)->sol::object {
            if (e.HasComponent<RigidBodyComponent>())
                return sol::make_object(lua, std::ref(e.GetComponent<RigidBodyComponent>()));
            auto& c = e.AddComponent<RigidBodyComponent>();
            c.Init();
            return sol::make_object(lua, std::ref(c));
            };

        g_getComponentFuncs["BoxColliderComponent"] = MakeGet<BoxColliderComponent>();
        g_hasComponentFuncs["BoxColliderComponent"] = MakeHas<BoxColliderComponent>();
        g_addComponentFuncs["BoxColliderComponent"] = [](Entity& e, sol::variadic_args, sol::state_view lua)->sol::object {
            auto& c = e.HasComponent<BoxColliderComponent>() ? e.GetComponent<BoxColliderComponent>()
                : e.AddComponent<BoxColliderComponent>();
            c.Init();
            return sol::make_object(lua, std::ref(c));
            };

        g_getComponentFuncs["ScriptComponent"] = MakeGet<ScriptComponent>();
        g_hasComponentFuncs["ScriptComponent"] = MakeHas<ScriptComponent>();
        g_addComponentFuncs["ScriptComponent"] = MakeAddSimple<ScriptComponent>();

        g_getComponentFuncs["AnimationComponent"] = MakeGet<AnimationComponent>();
        g_hasComponentFuncs["AnimationComponent"] = MakeHas<AnimationComponent>();
        g_addComponentFuncs["AnimationComponent"] = MakeAddOptionalArg<AnimationComponent, std::string>();

		m_UISystem = Renderer::Instance().m_SceneData.m_UI.get();

        BindInputToLua(m_Lua);
        BindMathToLua(m_Lua);
        BindFunctionToLua(m_Lua);
        BindEntityToLua(m_Lua);
        BindPhysicsToLua(m_Lua);
        BindUIToLua(m_Lua);
    }

    void ScriptSystem::RegisterEntityScript(ScriptComponent& scriptComponent)
    {
        entt::entity entity = scriptComponent.entt_entity;

        sol::environment env(m_Lua, sol::create, m_Lua.globals());

        sol::table publicTable = m_Lua.create_table();
        env["public"] = publicTable;

        Entity entityObject{ entity, Renderer::Instance().m_SceneData.m_Scene->GetRegistry() };
        env["entity"] = entityObject;
        env["self"] = entityObject;

        env.set_function("vec3", [](float x, float y, float z) {
            return glm::vec3(x, y, z);
            });

        try {
			std::filesystem::path p = AssetManager::Instance().ResolvePath(scriptComponent.scriptPath);
            m_Lua.script_file(p.generic_string(), env);

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
        scriptComponent.initialized = false;
        scriptComponent.startFunc = sol::nil;
        scriptComponent.updateFunc = sol::nil;
        scriptComponent.stopFunc = sol::nil;
        scriptComponent.environment = sol::nil;

    }

    void ScriptSystem::Update(double dt) {
        auto reg = Renderer::Instance().m_SceneData.m_Scene->GetRegistry();
        auto view = reg->GetRegistry().view<ScriptComponent>();

        for (auto e : view) {
            Entity entity{ e, reg };
            auto& script = view.get<ScriptComponent>(e);

            if (script.updateFunc.valid()) {
                try {
                    script.updateFunc(dt);
                }
                catch (const sol::error& err) {
                    std::cerr << "[Lua Runtime Error] " << err.what() << std::endl;
                }
            }
        }
    }


    void ScriptSystem::Start()
    {
        auto reg = Renderer::Instance().m_SceneData.m_Scene->GetRegistry();
        auto view = reg->GetRegistry().view<ScriptComponent>();

        for (auto e : view) {
            Entity entity{ e, Renderer::Instance().m_SceneData.m_Scene->GetRegistry() };
            auto& script = entity.GetComponent<ScriptComponent>();

            if (script.startFunc.valid()) {
                script.startFunc();
            }
        }
    }

    void ScriptSystem::Stop()
    {
        auto reg = Renderer::Instance().m_SceneData.m_Scene->GetRegistry();
        auto view = reg->GetRegistry().view<ScriptComponent>();

        for (auto e : view) {
            Entity entity{ e, Renderer::Instance().m_SceneData.m_Scene->GetRegistry() };
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

        lua_state.set_function("atan2", [](double y, double x) {
            return std::atan2(y, x);
            });

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
            auto* scene = Renderer::Instance().m_SceneData.m_Scene;
            if (!scene) return {};
            return scene->CreateEntity(name);
            });

        lua_state.set_function("setParent", [](Entity& child, Entity& parent) {
            if (!child.IsValid() || !parent.IsValid()) return;
            if (parent.HasComponent<HierarchyComponent>())
                parent.GetComponent<HierarchyComponent>().AddChild(parent.GetUUID(), child.GetUUID());
            });


        lua_state.set_function("log", [](const std::string& message) {
            std::cout << "[Lua] " << message << std::endl;
            });

        lua_state.set_function("getTime", []() {
            return Renderer::Instance().GetTime();
            });

        lua_state.set_function("getScene", []() -> Scene* {
            return Renderer::Instance().m_SceneData.m_Scene;
            });

        lua_state.set_function("destroyEntity", [](Entity& e) {
            if (e.IsValid()) Renderer::Instance().m_SceneData.m_Scene->DestroyEntity(e.GetUUID());
            });

        lua_state.set_function("removeEntityByName", [&lua_state](const std::string& name) -> sol::object {
            std::optional<Entity> ent = Renderer::Instance().m_SceneData.m_Scene->GetEntityByName(name);
            if (ent.has_value())
                Renderer::Instance().m_SceneData.m_Scene->DestroyEntity(ent->GetUUID());
            return sol::nil;
            });

        lua_state.set_function("getEntityByName", [&lua_state](const std::string& name) -> sol::object {
            std::optional<Entity> ent = Renderer::Instance().m_SceneData.m_Scene->GetEntityByName(name);
            if (ent.has_value())
                return sol::make_object(lua_state, ent.value());
            return sol::nil;
            });

        lua_state.set_function("attachScript", [this](Entity& e, const std::string& path) {
            if (!e.IsValid()) return;

            auto& sc = e.HasComponent<ScriptComponent>()
                ? e.GetComponent<ScriptComponent>()
                : e.AddComponent<ScriptComponent>();

            sc.scriptPath = path;
            sc.entt_entity = static_cast<entt::entity>(e);

            RegisterEntityScript(sc);

            if (sc.startFunc.valid()) {
                try { sc.startFunc(); }
                catch (const sol::error& err) {
                    std::cerr << "[Lua Runtime Error] " << err.what() << std::endl;
                }
            }
            });
    }

    void ScriptSystem::BindEntityToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<Entity>("Entity",
            "name", &Entity::GetName,
            "id", & Entity::GetUUID,
            "isValid", &Entity::IsValid
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

        lua_state.new_usertype<MeshComponent>("MeshComponent",
            sol::constructors<MeshComponent(), MeshComponent(const std::string&)>(),
            "getName", &MeshComponent::GetName,
            "hasMesh", &MeshComponent::HasMesh,
            "generateMesh", [](MeshComponent& mc,
                std::vector<float> verts,
                std::vector<unsigned int> inds) {
                    mc.GenerateMesh(verts, inds);
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

        lua_state.new_usertype<AnimationComponent>("AnimationComponent",
            "isPlaying", &AnimationComponent::IsPlaying,
            "isPaused", &AnimationComponent::IsPaused,
            "currentClipIndex", &AnimationComponent::CurrentClipIndex,
            "clipCount", &AnimationComponent::GetClipCount,
            "getTime", &AnimationComponent::GetTimeSeconds,
            "getSpeed", &AnimationComponent::GetSpeed,
            "getLoop", &AnimationComponent::GetLoop,
            "getInPlace", &AnimationComponent::GetInPlace,
            "getRootBoneName", &AnimationComponent::GetRootBoneName,

            "setTime", &AnimationComponent::SetTimeSeconds,
            "setSpeed", &AnimationComponent::SetSpeed,
            "setLoop", &AnimationComponent::SetLoop,
            "setInPlace", &AnimationComponent::SetInPlace,
            "setRootBoneName", &AnimationComponent::SetRootBoneName,
            "setModelAssetId", &AnimationComponent::SetModelAssetId,

            "stop", &AnimationComponent::Stop,
            "pause", &AnimationComponent::Pause,
            "resume", &AnimationComponent::Resume,

            "play", [](AnimationComponent& c, int clipIndex,
                sol::optional<bool> loop, sol::optional<float> speed) {
                    if (clipIndex < 0) return;
                    c.Play(static_cast<size_t>(clipIndex), loop.value_or(true), speed.value_or(1.0f));
            },

            "appendClipsFromAsset",
            [](AnimationComponent& c, const std::string& animAssetId,
                sol::optional<bool> dedupeByName, sol::optional<std::string> namePrefix) {
                    c.AppendClipsFromAsset(animAssetId, dedupeByName.value_or(true), namePrefix.value_or(""));
            },

            "playByName", [](AnimationComponent& c, const std::string& name,
                sol::optional<bool> loop, sol::optional<float> speed) {
                    const auto& clips = c.GetClips();
                    for (size_t i = 0; i < clips.size(); ++i) {
                        if (clips[i].name == name) {
                            c.Play(i, loop.value_or(true), speed.value_or(1.0f));
                            return true;
                        }
                    }
                    return false;
            },

            "getClipName", [](AnimationComponent& c, int index, sol::this_state ts) -> sol::object {
                sol::state_view lua(ts);
                if (index < 0) return sol::nil;
                size_t i = static_cast<size_t>(index);
                if (i >= c.GetClips().size()) return sol::nil;
                return sol::make_object(lua, c.GetClips()[i].name);
            }
        );
	}

    static QueryOptions ParseQueryOptions(const sol::object& obj) {
        QueryOptions o;
        if (!obj.valid() || obj.get_type() != sol::type::table) return o;
        sol::table t = obj.as<sol::table>();

        if (auto v = t.get<sol::optional<uint32_t>>("layer")) o.layer = *v;
        if (auto v = t.get<sol::optional<uint32_t>>("mask"))  o.mask = *v;
        if (auto v = t.get<sol::optional<bool>>("includeTriggers")) o.includeTriggers = *v;
        if (auto v = t.get<sol::optional<bool>>("bothSides"))       o.bothSides = *v;
        if (auto v = t.get<sol::optional<bool>>("precise"))         o.preciseSweep = *v;

        if (auto v = t.get<sol::optional<bool>>("hitNormal"); v && *v)     o.hitFlags |= physx::PxHitFlag::eNORMAL;
        if (auto v = t.get<sol::optional<bool>>("hitUV"); v && *v)         o.hitFlags |= physx::PxHitFlag::eUV;
        if (auto v = t.get<sol::optional<bool>>("hitFaceIndex"); v && *v)  o.hitFlags |= physx::PxHitFlag::eFACE_INDEX;

        return o;
    }

    void ScriptSystem::BindPhysicsToLua(sol::state& lua_state)
    {
        lua_state.new_usertype<CharacterControllerComponent>("CharacterController",
            "move", [&](CharacterControllerComponent& c, const glm::vec3& disp, float dt, sol::optional<float> minDist) {
                float md = minDist.value_or(0.001f);
                bool any = c.Move(disp, dt, md);
                sol::table t = lua_state.create_table();
                t["collided"] = any;
                t["grounded"] = c.IsGrounded();
                t["position"] = c.GetPosition();
                return t;
            },
            "set_position", &CharacterControllerComponent::SetPosition,
            "get_position", &CharacterControllerComponent::GetPosition,
            "resize_height", &CharacterControllerComponent::ResizeHeight,
            "set_radius", &CharacterControllerComponent::SetRadius,
            "set_step_offset", &CharacterControllerComponent::SetStepOffset,
            "set_slope_limit_deg", &CharacterControllerComponent::SetSlopeLimitDeg,
            "set_contact_offset", &CharacterControllerComponent::SetContactOffset,
            "set_layer_mask", &CharacterControllerComponent::SetLayerMask,
            "set_include_triggers", &CharacterControllerComponent::SetIncludeTriggers
        );

        sol::table character = lua_state.create_table();

        character.set_function("create_capsule", [&](const glm::vec3& startPos, sol::optional<float> radius,
            sol::optional<float> height, sol::optional<float> stepOffset,
            sol::optional<float> slopeLimitDeg, sol::optional<float> contactOffset) {
                auto& P = PhysicEngine::Instance();
                if (!P.HasScene() || !P.GetCCTManager()) return sol::make_object(lua_state, sol::nil);

                auto* mat = P.GetCCTMaterial();
                auto* mgr = P.GetCCTManager();

                CharacterControllerComponent* c = new CharacterControllerComponent();
                if (radius) c->SetRadius(*radius);
                if (height) c->ResizeHeight(*height);
                if (stepOffset) c->SetStepOffset(*stepOffset);
                if (slopeLimitDeg) c->SetSlopeLimitDeg(*slopeLimitDeg);
                if (contactOffset) c->SetContactOffset(*contactOffset);

                if (!c->Create(P.GetPhysics(), mgr, mat, startPos)) {
                    delete c;
                    return sol::make_object(lua_state, sol::nil);
                }
                return sol::make_object(lua_state, c);
            });

        character.set_function("destroy", [](CharacterControllerComponent* c) {
            if (!c) return;
            c->Destroy();
            delete c;
            });

        lua_state["character"] = character;

        lua_state.new_usertype<RigidBodyComponent>("RigidBody",
            "enableGravity", sol::property(
                [](RigidBodyComponent& c) { return c.enableGravity; },
                [](RigidBodyComponent& c, bool v) { c.enableGravity = v; c.UpdateEnableGravity(); }
            ),
            "set_body_type", [](RigidBodyComponent& c, const std::string& t) { c.bodyTypeString = t; c.UpdateBodyType(); },
            "set_linear_damping", [](RigidBodyComponent& c, float v) { c.linearDamping = v; c.UpdateDamping(); },
            "set_angular_damping", [](RigidBodyComponent& c, float v) { c.angularDamping = v; c.UpdateDamping(); },
            "enable_gravity", &RigidBodyComponent::enableGravity
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
                if (queryStatic && !queryStatic.value())   f.flags &= ~physx::PxQueryFlag::eSTATIC;

                physx::PxRaycastBuffer hit;
                const bool ok = scene->raycast(ToPx(origin), ToPx(glm::normalize(dir)), maxDist,
                    hit, physx::PxHitFlag::eDEFAULT, f);

                sol::table res = lua_state.create_table();
                res["hit"] = ok && hit.hasBlock;
                if (ok && hit.hasBlock) {
                    res["position"] = ToGlm(hit.block.position);
                    res["normal"] = ToGlm(hit.block.normal);
                    res["distance"] = hit.block.distance;
                    res["entity"] = PhysicEngine::Instance().ActorToEntityObject(hit.block.actor, lua_state);
                    res["shape"] = (uintptr_t)hit.block.shape;
                }
                return sol::make_object(lua_state, res);
            });

        physics.set_function("raycast_all",
            [&lua_state](const glm::vec3& origin,
                const glm::vec3& dir,
                float maxDist,
                sol::object opts)
            {
                auto& P = PhysicEngine::Instance();
                std::vector<physx::PxRaycastHit> hits;
                QueryOptions q = ParseQueryOptions(opts);
                P.RaycastAll(ToPx(origin), ToPx(dir), maxDist, hits, q);

                sol::table out = lua_state.create_table(static_cast<int>(hits.size()), 0);
                int i = 1;
                for (const auto& h : hits) {
                    sol::table t = lua_state.create_table();
                    t["distance"] = h.distance;
                    t["position"] = ToGlm(h.position);
                    t["normal"] = ToGlm(h.normal);
                    t["faceIndex"] = h.faceIndex;
                    t["entity"] = PhysicEngine::Instance().ActorToEntityObject(h.actor, lua_state);
                    t["shape"] = (uintptr_t)h.shape;
                    out[i++] = t;
                }
                return out;
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
                    auto obj = PhysicEngine::Instance().ActorToEntityObject(a, lua_state);
                    if (obj != sol::nil) ents[idx++] = obj;
                }

                sol::table t = lua_state.create_table();
                t["entities"] = ents;
                t["count"] = idx - 1;
                return sol::make_object(lua_state, t);
            });

        physics.set_function("overlap_box",
            [&lua_state](const glm::vec3& center, const glm::quat& rot, const glm::vec3& halfExtents, sol::object opts)
            {
                auto& P = PhysicEngine::Instance();
                std::vector<physx::PxOverlapHit> hits;
                QueryOptions q = ParseQueryOptions(opts);
                P.OverlapBox(ToPx(halfExtents), ToPx(center, rot), hits, q);

                sol::table out = lua_state.create_table(static_cast<int>(hits.size()), 0);
                int i = 1;
                for (const auto& h : hits) {
                    sol::table t = lua_state.create_table();
                    t["entity"] = PhysicEngine::Instance().ActorToEntityObject(h.actor, lua_state);
                    t["shape"] = (uintptr_t)h.shape;
                    out[i++] = t;
                }
                return out;
            });

        physics.set_function("overlap_capsule",
            [&lua_state](const glm::vec3& center, const glm::quat& rot, float radius, float halfHeight, sol::object opts)
            {
                auto& P = PhysicEngine::Instance();
                std::vector<physx::PxOverlapHit> hits;
                QueryOptions q = ParseQueryOptions(opts);
                P.OverlapCapsule(radius, halfHeight, ToPx(center, rot), hits, q);

                sol::table out = lua_state.create_table(static_cast<int>(hits.size()), 0);
                int i = 1;
                for (const auto& h : hits) {
                    sol::table t = lua_state.create_table();
                    t["entity"] = PhysicEngine::Instance().ActorToEntityObject(h.actor, lua_state);
                    t["shape"] = (uintptr_t)h.shape;
                    out[i++] = t;
                }
                return out;
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
                    res["entity"] = PhysicEngine::Instance().ActorToEntityObject(buf.block.actor, lua_state);
                    res["shape"] = (uintptr_t)buf.block.shape;
                }
                return sol::make_object(lua_state, res);
            });

        physics.set_function("sweep_box",
            [&lua_state](const glm::vec3& center, const glm::quat& rot, const glm::vec3& halfExtents,
                const glm::vec3& dir, float maxDist, sol::object opts) -> sol::object
            {
                auto& P = PhysicEngine::Instance();
                physx::PxSweepHit hit;
                QueryOptions q = ParseQueryOptions(opts);
                bool ok = P.SweepBox(ToPx(halfExtents), ToPx(center, rot), ToPx(dir), maxDist, hit, q);
                if (!ok) return sol::make_object(lua_state, sol::nil);
                sol::table t = lua_state.create_table();
                t["distance"] = hit.distance;
                t["position"] = ToGlm(hit.position);
                t["normal"] = ToGlm(hit.normal);
                t["faceIndex"] = hit.faceIndex;
                t["entity"] = PhysicEngine::Instance().ActorToEntityObject(hit.actor, lua_state);
                t["shape"] = (uintptr_t)hit.shape;
                return sol::make_object(lua_state, t);
            });

        physics.set_function("sweep_capsule",
            [&lua_state](const glm::vec3& center, const glm::quat& rot, float radius, float halfHeight,
                const glm::vec3& dir, float maxDist, sol::object opts) -> sol::object
            {
                auto& P = PhysicEngine::Instance();
                physx::PxSweepHit hit;
                QueryOptions q = ParseQueryOptions(opts);
                bool ok = P.SweepCapsule(radius, halfHeight, ToPx(center, rot), ToPx(dir), maxDist, hit, q);
                if (!ok) return sol::make_object(lua_state, sol::nil);
                sol::table t = lua_state.create_table();
                t["distance"] = hit.distance;
                t["position"] = ToGlm(hit.position);
                t["normal"] = ToGlm(hit.normal);
                t["faceIndex"] = hit.faceIndex;
                t["entity"] = PhysicEngine::Instance().ActorToEntityObject(hit.actor, lua_state);
                t["shape"] = (uintptr_t)hit.shape;
                return sol::make_object(lua_state, t);
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

        physics.set_function("set_rotation",
            [](Entity& e, const glm::vec3& eulerRad)
            {
                if (!e.HasComponent<RigidBodyComponent>()) return;
                auto& rbc = e.GetComponent<RigidBodyComponent>();

                if (physx::PxRigidDynamic* dyn = rbc.GetDynamic())
                {
                    physx::PxTransform pose = dyn->getGlobalPose();
                    glm::quat q = glm::quat(eulerRad);
                    physx::PxQuat pxq(q.x, q.y, q.z, q.w);
                    pose.q = pxq.getNormalized();

                    if (dyn->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
                        dyn->setKinematicTarget(pose);
                    else
                        dyn->setGlobalPose(pose, true);

                    dyn->wakeUp();
                }
            });

        physics.set_function("get_linear_velocity",
            [](Entity& e) -> glm::vec3
            {
                if (!e.HasComponent<RigidBodyComponent>()) return glm::vec3(0.0f);
                auto& rbc = e.GetComponent<RigidBodyComponent>();
                if (physx::PxRigidDynamic* dyn = rbc.GetDynamic())
                    return ToGlm(dyn->getLinearVelocity());
                return glm::vec3(0.0f);
            });

        physics.set_function("set_angular_velocity",
            [](Entity& e, const glm::vec3& wRadPerSec)
            {
                if (!e.HasComponent<RigidBodyComponent>()) return;
                auto& rbc = e.GetComponent<RigidBodyComponent>();
                if (physx::PxRigidDynamic* dyn = rbc.GetDynamic())
                {
                    dyn->setAngularVelocity(ToPx(wRadPerSec), true);
                    dyn->wakeUp();
                }
            });

        physics.set_function("move_kinematic",
            [](Entity& e, const glm::vec3& targetPos, const glm::quat& targetRot, sol::optional<bool> doSweep) {
                if (!e.HasComponent<RigidBodyComponent>()) return false;
                auto& rb = e.GetComponent<RigidBodyComponent>();
                bool sweep = doSweep.value_or(true);
                return rb.MoveKinematic(targetPos, targetRot, sweep);
            });

        physics["CombineMode"] = lua_state.create_table_with(
            "Average", (int)physx::PxCombineMode::eAVERAGE,
            "Min", (int)physx::PxCombineMode::eMIN,
            "Multiply", (int)physx::PxCombineMode::eMULTIPLY,
            "Max", (int)physx::PxCombineMode::eMAX
        );

        lua_state["physics"] = physics;

        lua_state.new_usertype<BoxColliderComponent>("BoxColliderComponent",
            "set_trigger", &BoxColliderComponent::SetTrigger,
            "is_trigger", &BoxColliderComponent::IsTrigger,
            "set_local_pose", &BoxColliderComponent::SetLocalPose,
            "set_combine_modes", &BoxColliderComponent::SetMaterialCombineModes,
            "set_query_filter", &BoxColliderComponent::SetQueryFilter,

            "mass", & BoxColliderComponent::mass,
            "friction", & BoxColliderComponent::friction,
            "bounciness", & BoxColliderComponent::bounciness,
            "useEntityScale", & BoxColliderComponent::m_UseEntityScale,
            "size", & BoxColliderComponent::m_Size,
            "Init", & BoxColliderComponent::Init
        );


        lua_state.new_usertype<SphereColliderComponent>("SphereColliderComponent",
            "set_trigger", &SphereColliderComponent::SetTrigger,
            "is_trigger", &SphereColliderComponent::IsTrigger,
            "set_local_pose", &SphereColliderComponent::SetLocalPose,
            "set_combine_modes", &SphereColliderComponent::SetMaterialCombineModes,
            "set_query_filter", &SphereColliderComponent::SetQueryFilter
        );

        lua_state.new_usertype<CapsuleColliderComponent>("CapsuleColliderComponent",
            "set_trigger", &CapsuleColliderComponent::SetTrigger,
            "is_trigger", &CapsuleColliderComponent::IsTrigger,
            "set_local_pose", &CapsuleColliderComponent::SetLocalPose,
            "set_combine_modes", &CapsuleColliderComponent::SetMaterialCombineModes,
            "set_query_filter", &CapsuleColliderComponent::SetQueryFilter
        );
    }
    
    void ScriptSystem::BindUIToLua(sol::state& L)
    {
        L.new_usertype<UIElement>("UIElement", sol::no_constructor,
            "addChild", [](std::shared_ptr<UIElement>& self, std::shared_ptr<UIElement> child) {
                if (child) self->AddChild(child);
            },
            
            "set_pos", [](std::shared_ptr<UIElement>& self, float x, float y) { self->Transform().pos = { x, y }; },
            "set_size", [](std::shared_ptr<UIElement>& self, float w, float h) { self->Transform().size = { w, h }; },
            "rect", [](std::shared_ptr<UIElement>& self) {
                const auto& r = self->Transform().rect;
                return std::make_tuple(r.x, r.y, r.w, r.h);
            },
            
            "set_bg", [](std::shared_ptr<UIElement>& self, float r, float g, float b, float a) { self->Style().bg = { r,g,b,a }; },
            "set_fg", [](std::shared_ptr<UIElement>& self, float r, float g, float b, float a) { self->Style().fg = { r,g,b,a }; },
            "set_padding", [](std::shared_ptr<UIElement>& self, float p) { self->Style().padding = p; },
            
            "set_focusable", [](std::shared_ptr<UIElement>& self, bool f) { self->SetFocusable(f); },
            "set_tab_index", [](std::shared_ptr<UIElement>& self, int i) { self->SetTabIndex(i); },
            "set_enabled", [](std::shared_ptr<UIElement>& self, bool e) { self->SetEnabled(e); }
        );

        L.new_usertype<UIContainer>("UIContainer", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_layout", [](std::shared_ptr<UIContainer>& c, const std::string& layout) {
                c->layout = (layout == "horizontal" || layout == "Horizontal")
                    ? UILayoutType::Horizontal : UILayoutType::Vertical;
            },
            "set_gap", [](std::shared_ptr<UIContainer>& c, float gap) { c->gap = gap; }
        );

        L.new_usertype<UIButton>("UIButton", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_label", [](std::shared_ptr<UIButton>& b, const std::string& s) { b->label = s; },
            "on_click", [](std::shared_ptr<UIButton>& b, sol::function fn) {
                b->onClick = [fn]() mutable { if (fn.valid()) fn(); };
            }
        );

        L.new_usertype<UIText>("UIText", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_text", [](std::shared_ptr<UIText>& t, const std::string& s) { t->text = s; }
        );

        L.new_usertype<UITextInput>("UITextInput", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_text", [](std::shared_ptr<UITextInput>& t, const std::string& s) { t->text = s; },
            "get_text", [](std::shared_ptr<UITextInput>& t) { return t->text; }
        );

        L.new_usertype<UICheckbox>("UICheckbox", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_label", [](std::shared_ptr<UICheckbox>& c, const std::string& s) { c->label = s; },
            "set_checked", [](std::shared_ptr<UICheckbox>& c, bool v) { c->checked = v; },
            "is_checked", [](std::shared_ptr<UICheckbox>& c) { return c->checked; }
        );

        L.new_usertype<UISlider>("UISlider", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_range", [](std::shared_ptr<UISlider>& s, float min, float max) { s->min = min; s->max = max; },
            "set_value", [](std::shared_ptr<UISlider>& s, float v) { s->value = v; },
            "get_value", [](std::shared_ptr<UISlider>& s) { return s->value; }
        );

        L.new_usertype<UIProgressBar>("UIProgressBar", sol::no_constructor,
            sol::base_classes, sol::bases<UIElement>(),
            "set_range", [](std::shared_ptr<UIProgressBar>& p, float min, float max) { p->min = min; p->max = max; },
            "set_value", [](std::shared_ptr<UIProgressBar>& p, float v) { p->value = v; }
        );

        sol::table ui = L.create_table();

        ui.set_function("set_root", [this](std::shared_ptr<UIElement> root) {
            if (m_UISystem && root) {
                m_UISystem->SetRoot(std::move(root));
            }
            });

        ui.set_function("container", [](sol::optional<std::string> id, sol::optional<std::string> layout) {
            auto c = std::make_shared<UIContainer>(id.value_or("container"));
            if (layout) c->layout = (*layout == "horizontal" || *layout == "Horizontal")
                ? UILayoutType::Horizontal : UILayoutType::Vertical;
            return c;
            });

        ui.set_function("text", [](sol::optional<std::string> id, sol::optional<std::string> value) {
            auto t = std::make_shared<UIText>(id.value_or("text"));
            t->text = value.value_or("");
            return t;
            });

        ui.set_function("text_input", [](sol::optional<std::string> id, sol::optional<std::string> value) {
            auto t = std::make_shared<UITextInput>(id.value_or("textinput"));
            if (value) t->text = *value;
            return t;
            });

        ui.set_function("button", [](sol::optional<std::string> id,
            sol::optional<std::string> label,
            sol::optional<sol::function> cb) {
                auto b = std::make_shared<UIButton>(id.value_or("button"));
                if (label) b->label = *label;
                if (cb && cb->valid()) {
                    sol::function f = *cb;
                    b->onClick = [f]() mutable { if (f.valid()) f(); };
                }
                return b;
            });

        ui.set_function("checkbox", [](sol::optional<std::string> id,
            sol::optional<std::string> label,
            sol::optional<bool> checked) {
                auto c = std::make_shared<UICheckbox>(id.value_or("checkbox"));
                if (label)   c->label = *label;
                if (checked) c->checked = *checked;
                return c;
            });

        ui.set_function("slider", [](sol::optional<std::string> id,
            sol::optional<float> min,
            sol::optional<float> max,
            sol::optional<float> value) {
                auto s = std::make_shared<UISlider>(id.value_or("slider"));
                if (min)   s->min = *min;
                if (max)   s->max = *max;
                if (value) s->value = *value;
                return s;
            });

        ui.set_function("progress", [](sol::optional<std::string> id,
            sol::optional<float> min,
            sol::optional<float> max,
            sol::optional<float> value) {
                auto p = std::make_shared<UIProgressBar>(id.value_or("progress"));
                if (min)   p->min = *min;
                if (max)   p->max = *max;
                if (value) p->value = *value;
                return p;
            });

        L["ui"] = ui;
    }
}