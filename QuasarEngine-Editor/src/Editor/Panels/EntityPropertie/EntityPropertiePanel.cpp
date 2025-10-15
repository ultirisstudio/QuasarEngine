#include "EntityPropertiePanel.h"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <algorithm>
#include <cctype>

#include <Editor/SceneManager.h>
#include <Editor/Panels/SceneHierarchy/SceneHierarchy.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/AllComponents.h>
#include <QuasarEngine/Core/UUID.h>

#include <Editor/Panels/Components/TransformComponentPanel.h>
#include <Editor/Panels/Components/CameraComponentPanel.h>
#include <Editor/Panels/Components/MeshComponentPanel.h>
#include <Editor/Panels/Components/TerrainComponentPanel.h>
#include <Editor/Panels/Components/MaterialComponentPanel.h>
#include <Editor/Panels/Components/LightComponentPanel.h>
#include <Editor/Panels/Components/MeshRendererComponentPanel.h>

#include <Editor/Panels/Components/Physics/RigidBodyComponentPanel.h>
#include <Editor/Panels/Components/Physics/BoxColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/ConvexMeshColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/TriangleMeshColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/CapsuleColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/PlaneColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/SphereColliderComponentPanel.h>
#include <Editor/Panels/Components/Physics/HeightfieldColliderComponentPanel.h>

#include <Editor/Panels/Components/Scripting/ScriptComponentPanel.h>

#include <Editor/Panels/Components/Animation/AnimationComponentPanel.h>

#include <QuasarEngine/Entity/AllComponents.h>
#include <QuasarEngine/Core/UUID.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
    /*struct TransformPanelAdapter : public IComponentPanel
    {
        TransformComponentPanel p;
        const char* Name() const override { return "Transform"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct CameraPanelAdapter : public IComponentPanel
    {
        CameraComponentPanel p;
        const char* Name() const override { return "Camera"; }
        void Render(Entity& e, Scene* s) override { p.Render(e, *s); }
    };
    struct MeshRendererPanelAdapter : public IComponentPanel
    {
        MeshRendererComponentPanel p;
        const char* Name() const override { return "Mesh Renderer"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct MeshPanelAdapter : public IComponentPanel
    {
        MeshComponentPanel p;
        const char* Name() const override { return "Mesh"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct TerrainPanelAdapter : public IComponentPanel
    {
        TerrainComponentPanel p;
        const char* Name() const override { return "Terrain"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct MaterialPanelAdapter : public IComponentPanel
    {
        MaterialComponentPanel p;
        const char* Name() const override { return "Material"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct LightPanelAdapter : public IComponentPanel
    {
        LightComponentPanel p;
        const char* Name() const override { return "Light"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct RigidBodyPanelAdapter : public IComponentPanel
    {
        RigidBodyComponentPanel p;
        const char* Name() const override { return "RigidBody"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct BoxColliderPanelAdapter : public IComponentPanel
    {
        BoxColliderComponentPanel p;
        const char* Name() const override { return "Box Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct SphereColliderPanelAdapter : public IComponentPanel
    {
        SphereColliderComponentPanel p;
        const char* Name() const override { return "Sphere Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct CapsuleColliderPanelAdapter : public IComponentPanel
    {
        CapsuleColliderComponentPanel p;
        const char* Name() const override { return "Capsule Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct PlaneColliderPanelAdapter : public IComponentPanel
    {
        PlaneColliderComponentPanel p;
        const char* Name() const override { return "Plane Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct ConvexMeshColliderPanelAdapter : public IComponentPanel
    {
        ConvexMeshColliderComponentPanel p;
        const char* Name() const override { return "Convex Mesh Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct TriangleMeshColliderPanelAdapter : public IComponentPanel
    {
        TriangleMeshColliderComponentPanel p;
        const char* Name() const override { return "Triangle Mesh Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct HeightfieldColliderPanelAdapter : public IComponentPanel
    {
        HeightfieldColliderComponentPanel p;
        const char* Name() const override { return "Heightfield Collider"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct ScriptPanelAdapter : public IComponentPanel
    {
        explicit ScriptPanelAdapter(const std::string& projectPath) : p(projectPath) {}
        ScriptComponentPanel p;
        const char* Name() const override { return "Script"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
    };
    struct AnimationPanelAdapter : public IComponentPanel
    {
        AnimationComponentPanel p;
        const char* Name() const override { return "Animation"; }
        void Render(Entity& e, Scene*) override { p.Render(e); }
	};*/

    static std::string toLower(std::string s)
    {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    bool EntityPropertiePanel::textContainsI(const std::string& hay, const std::string& needle)
    {
        if (needle.empty()) return true;
        return toLower(hay).find(toLower(needle)) != std::string::npos;
    }

    EntityPropertiePanel::EntityPropertiePanel(const std::string& projectPath)
        : m_ProjectPath(projectPath)
    {
        buildPanels(projectPath);
        buildMenuItems(projectPath);
    }

    EntityPropertiePanel::~EntityPropertiePanel()
    {
        AssetManager::Instance().unloadAsset("no_texture.png");
    }

    void EntityPropertiePanel::buildPanels(const std::string& projectPath)
    {
        m_Panels.reserve(12);
        m_Panels.push_back({ std::make_unique<TransformComponentPanel>(), "Transform" });
        m_Panels.push_back({ std::make_unique<CameraComponentPanel>(), "Camera" });
        m_Panels.push_back({ std::make_unique<MeshRendererComponentPanel>(), "Mesh Renderer" });
        m_Panels.push_back({ std::make_unique<MeshComponentPanel>(), "Mesh" });
        m_Panels.push_back({ std::make_unique<TerrainComponentPanel>(), "Terrain" });
        m_Panels.push_back({ std::make_unique<MaterialComponentPanel>(), "Material" });
        m_Panels.push_back({ std::make_unique<LightComponentPanel>(), "Light" });
        m_Panels.push_back({ std::make_unique<RigidBodyComponentPanel>(), "RigidBody" });
        m_Panels.push_back({ std::make_unique<BoxColliderComponentPanel>(), "Box Collider" });
        m_Panels.push_back({ std::make_unique<SphereColliderComponentPanel>(), "Sphere Collider" });
        m_Panels.push_back({ std::make_unique<CapsuleColliderComponentPanel>(), "Capsule Collider" });
        m_Panels.push_back({ std::make_unique<PlaneColliderComponentPanel>(), "Plane Collider" });
        m_Panels.push_back({ std::make_unique<ConvexMeshColliderComponentPanel>(), "Convex Mesh Collider" });
        m_Panels.push_back({ std::make_unique<TriangleMeshColliderComponentPanel>(), "Triangle Mesh Collider" });
        m_Panels.push_back({ std::make_unique<HeightfieldColliderComponentPanel>(), "Heightfield Collider" });
        m_Panels.push_back({ std::make_unique<ScriptComponentPanel>(projectPath), "Script" });
		m_Panels.push_back({ std::make_unique<AnimationComponentPanel>(), "Animation" });
    }

    void EntityPropertiePanel::buildMenuItems(const std::string& projectPath)
    {
        m_MenuItems.clear();
        m_MenuItems.reserve(16);

        auto add = [&](std::string name,
            std::string category,
            std::string keywords,
            std::function<bool(Entity&)> hasFn,
            std::function<void(Entity&)> addFn)
            {
                m_MenuItems.push_back(MenuItem{ std::move(name), std::move(hasFn), std::move(addFn),
                                                std::move(category), std::move(keywords) });
            };

        add("Transform Component", "Core", "transform position rotation scale",
            [](Entity& e) { return e.HasComponent<TransformComponent>(); },
            [](Entity& e) { e.AddComponent<TransformComponent>(); });

        add("Mesh Renderer Component", "Rendering", "mesh renderer material render",
            [](Entity& e) { return e.HasComponent<MeshRendererComponent>(); },
            [](Entity& e) { e.AddComponent<MeshRendererComponent>(); });

        add("Mesh Component", "Rendering", "mesh model geometry",
            [](Entity& e) { return e.HasComponent<MeshComponent>(); },
            [](Entity& e) {
                if (!e.HasComponent<MeshRendererComponent>()) e.AddComponent<MeshRendererComponent>();
                e.AddComponent<MeshComponent>();
            });

        add("Material Component", "Rendering", "material pbr shader",
            [](Entity& e) { return e.HasComponent<MaterialComponent>(); },
            [](Entity& e) { e.AddComponent<MaterialComponent>(); });

        add("Terrain Component", "Terrain", "terrain heightmap ground",
            [](Entity& e) { return e.HasComponent<TerrainComponent>(); },
            [](Entity& e) { e.AddComponent<TerrainComponent>(); });

        add("Camera Component", "Camera", "camera perspective orthographic",
            [](Entity& e) { return e.HasComponent<CameraComponent>(); },
            [](Entity& e) {
                auto& c = e.AddComponent<CameraComponent>();
                if (e.HasComponent<TransformComponent>())
                    c.GetCamera().Init(&e.GetComponent<TransformComponent>());
            });

        add("Directional Light", "Lighting", "light directional sun shadow",
            [](Entity& e) { return e.HasComponent<LightComponent>(); },
            [](Entity& e) { e.AddComponent<LightComponent>(LightComponent::LightType::DIRECTIONAL); });

        add("Point Light Component", "Lighting", "light point spot",
            [](Entity& e) { return e.HasComponent<LightComponent>(); },
            [](Entity& e) { e.AddComponent<LightComponent>(LightComponent::LightType::POINT); });

        add("RigidBody Component", "Physics", "rigidbody physics body",
            [](Entity& e) { return e.HasComponent<RigidBodyComponent>(); },
            [](Entity& e) { e.AddComponent<RigidBodyComponent>().Init(); });

        add("Box Collider Component", "Physics", "collider box cube",
            [](Entity& e) { return e.HasComponent<BoxColliderComponent>(); },
            [](Entity& e) { e.AddComponent<BoxColliderComponent>().Init(); });

        add("Sphere Collider Component", "Physics", "collider sphere ball",
            [](Entity& e) { return e.HasComponent<SphereColliderComponent>(); },
            [](Entity& e) { e.AddComponent<SphereColliderComponent>().Init(); });

        add("Capsule Collider Component", "Physics", "collider capsule character",
            [](Entity& e) { return e.HasComponent<CapsuleColliderComponent>(); },
            [](Entity& e) { e.AddComponent<CapsuleColliderComponent>().Init(); });

        add("Plane Collider Component", "Physics", "collider plane infinite ground wall",
            [](Entity& e) { return e.HasComponent<PlaneColliderComponent>(); },
            [](Entity& e) { e.AddComponent<PlaneColliderComponent>().Init(); });

        add("Convex Mesh Collider Component", "Physics", "collider convex hull dynamic",
            [](Entity& e) { return e.HasComponent<ConvexMeshColliderComponent>(); },
            [](Entity& e) { e.AddComponent<ConvexMeshColliderComponent>().Init(); });

        add("Triangle Mesh Collider Component", "Physics", "collider triangle mesh static concave",
            [](Entity& e) { return e.HasComponent<TriangleMeshColliderComponent>(); },
            [](Entity& e) { e.AddComponent<TriangleMeshColliderComponent>().Init(); });

        add("Heightfield Collider Component", "Physics", "collider terrain heightfield ground",
            [](Entity& e) { return e.HasComponent<HeightfieldColliderComponent>(); },
            [](Entity& e) { e.AddComponent<HeightfieldColliderComponent>().Init(); });

        add("Scripting Component", "Scripting", "script csharp lua behavior",
            [](Entity& e) { return e.HasComponent<ScriptComponent>(); },
            [](Entity& e) { e.AddComponent<ScriptComponent>(); });

		add("Animation Component", "Animation", "animation animator clip",
			[](Entity& e) { return e.HasComponent<AnimationComponent>(); },
			[](Entity& e) { e.AddComponent<AnimationComponent>(); });
    }

    void EntityPropertiePanel::renderPanels(Entity entity)
    {
        for (auto& entry : m_Panels)
            entry.panel->Render(entity);
    }

    void EntityPropertiePanel::renderAddComponentPopup(Entity entity)
    {
        if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputTextWithHint("##search_components", "Search components...", m_SearchBuffer, sizeof(m_SearchBuffer));

            const std::string filter = m_SearchBuffer;

            std::vector<std::string> categories;
            categories.reserve(16);
            for (auto& it : m_MenuItems)
                if (std::find(categories.begin(), categories.end(), it.category) == categories.end())
                    categories.push_back(it.category);

            std::sort(categories.begin(), categories.end());

            for (const auto& cat : categories)
            {
                std::vector<const MenuItem*> items;
                for (auto& it : m_MenuItems)
                {
                    if (it.category != cat) continue;
                    if (it.hasComponent(entity)) continue;
                    if (!textContainsI(it.name + " " + it.keywords, filter)) continue;
                    items.push_back(&it);
                }
                if (items.empty()) continue;

                if (ImGui::BeginMenu(cat.c_str()))
                {
                    for (auto* it : items)
                    {
                        if (ImGui::MenuItem(it->name.c_str()))
                        {
                            it->addComponent(entity);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::EndPopup();
        }
    }

    void EntityPropertiePanel::OnImGuiRender(SceneHierarchy& sceneHierarchy)
    {
        ImGui::Begin("Inspector");

        if (sceneHierarchy.m_SelectedEntity.IsValid())
        {
            Entity entity = sceneHierarchy.m_SelectedEntity;
            UUID uuid = entity.GetUUID();

            ImGui::Text("UUID: %llu", (unsigned long long)uuid);
            ImGui::Separator();

            if (entity.HasComponent<TagComponent>())
            {
                auto& tc = entity.GetComponent<TagComponent>();

                char buffer[256];
                std::strncpy(buffer, entity.GetName().c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';
                if (ImGui::InputText(("##" + uuid.ToString()).c_str(), buffer, sizeof(buffer)))
                {
                    entity.GetComponent<TagComponent>().Tag = std::string(buffer);
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Tags");

                static char s_TagSearch[64] = { 0 };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##search_tags", "Rechercher un tag...", s_TagSearch, sizeof(s_TagSearch));
                const std::string tagFilter = s_TagSearch;

                struct TagOption { const char* label; TagMask flag; const char* keywords; };
                static const TagOption kTagOptions[] = {
                    {"Player",      TagMask::Player,      "player hero character"},
                    {"Enemy",       TagMask::Enemy,       "enemy hostile foe"},
                    {"NPC",         TagMask::NPC,         "npc civilian vendor"},
                    {"Collectible", TagMask::Collectible, "pickup item loot"},
                    {"Trigger",     TagMask::Trigger,     "trigger volume area"},
                    {"Static",      TagMask::Static,      "static environment"},
                    {"Dynamic",     TagMask::Dynamic,     "dynamic movable"},
                    {"Boss",        TagMask::Boss,        "boss elite"},
                    {"Projectile",  TagMask::Projectile,  "projectile bullet"},
                };

                if (ImGui::Button("Tout cocher")) { for (auto& o : kTagOptions) tc.Add(o.flag); }
                ImGui::SameLine();
                if (ImGui::Button("Tout decocher")) { for (auto& o : kTagOptions) tc.Mask = TagMask::None; }

                const int columns = 2;
                if (ImGui::BeginTable("##tag_table", columns, ImGuiTableFlags_SizingStretchProp))
                {
                    int col = 0;
                    for (const auto& opt : kTagOptions)
                    {
                        auto containsI = [](const std::string& hay, const std::string& needle) {
                            if (needle.empty()) return true;
                            auto toLower = [](std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; };
                            return toLower(hay).find(toLower(needle)) != std::string::npos;
                            };
                        if (!containsI(std::string(opt.label) + " " + opt.keywords, tagFilter))
                            continue;

                        if (col == 0) ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(col);

                        bool checked = tc.HasAny(opt.flag);
                        if (ImGui::Checkbox(opt.label, &checked))
                        {
                            if (checked) tc.Add(opt.flag);
                            else         tc.Remove(opt.flag);
                        }

                        col = (col + 1) % columns;
                    }
                    ImGui::EndTable();
                }
            }

            ImGui::Separator();

            renderPanels(entity);
            renderAddComponentPopup(entity);
        }

        ImGui::End();
    }
}