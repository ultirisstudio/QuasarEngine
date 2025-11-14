#include "qepch.h"
#include "SceneSerializer.h"

#include "SceneObject.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/AllComponents.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Core/UUID.h>
#include "QuasarEngine/Tools/Utils.h"

#include "Importer/TextureConfigImporter.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <fstream>
#include <sstream>
#include <locale>
#include <clocale>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace QuasarEngine
{
    static const std::unordered_map<std::string, QuasarEngine::TagMask> kTagByName = {
        {"Player",      QuasarEngine::TagMask::Player},
        {"Enemy",       QuasarEngine::TagMask::Enemy},
        {"NPC",         QuasarEngine::TagMask::NPC},
        {"Collectible", QuasarEngine::TagMask::Collectible},
        {"Trigger",     QuasarEngine::TagMask::Trigger},
        {"Static",      QuasarEngine::TagMask::Static},
        {"Dynamic",     QuasarEngine::TagMask::Dynamic},
        {"Boss",        QuasarEngine::TagMask::Boss},
        {"Projectile",  QuasarEngine::TagMask::Projectile},
    };

    static std::vector<std::string> MaskToNames(QuasarEngine::TagMask mask)
    {
        using namespace QuasarEngine;
        std::vector<std::string> out;
        const uint64_t bits = static_cast<uint64_t>(mask);
        for (auto& kv : kTagByName)
        {
            const uint64_t bit = static_cast<uint64_t>(kv.second);
            if ((bits & bit) != 0ull) out.push_back(kv.first);
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    static std::string U64ToHex(uint64_t v)
    {
        std::ostringstream oss;
        oss << "0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << v;
        return oss.str();
    }

    static bool HexToU64(const std::string& s, uint64_t& out)
    {
        std::string t = s;
        if (t.rfind("0x", 0) == 0 || t.rfind("0X", 0) == 0) t = t.substr(2);
        if (t.empty()) return false;
        char* end = nullptr;
        out = std::strtoull(t.c_str(), &end, 16);
        return end && *end == '\0';
    }

    static std::string trim(const std::string& s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a])) ++a;
        while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
        return s.substr(a, b - a);
    }

    static QuasarEngine::TagMask NamesToMask(const YAML::Node& node)
    {
        using namespace QuasarEngine;
        uint64_t bits = 0;

        if (node && node.IsSequence())
        {
            for (auto n : node)
            {
                if (!n.IsScalar()) continue;
                std::string key = n.as<std::string>();
                auto it = kTagByName.find(key);
                if (it != kTagByName.end())
                    bits |= static_cast<uint64_t>(it->second);
            }
        }
        return static_cast<TagMask>(bits);
    }

    static QuasarEngine::TagMask NodeToMask(const YAML::Node& node)
    {
        using namespace QuasarEngine;
        if (!node) return TagMask::None;

        if (node.IsSequence())
            return NamesToMask(node);

        if (node.IsScalar())
        {
            const std::string s = node.as<std::string>();
            
            uint64_t u = 0;
            if (HexToU64(s, u)) return static_cast<TagMask>(u);

            bool isDec = !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
            if (isDec) return static_cast<TagMask>(std::strtoull(s.c_str(), nullptr, 10));

            uint64_t bits = 0;
            std::stringstream ss(s);
            std::string part;
            while (std::getline(ss, part, '|'))
            {
                part = trim(part);
                auto it = kTagByName.find(part);
                if (it != kTagByName.end())
                    bits |= static_cast<uint64_t>(it->second);
            }
            return static_cast<TagMask>(bits);
        }

        return TagMask::None;
    }

    struct NumericLocaleGuard {
        std::string prevC;
        std::locale prevCpp;
        NumericLocaleGuard()
            : prevCpp(std::locale())
        {
            const char* cur = std::setlocale(LC_NUMERIC, nullptr);
            if (cur) prevC = cur;
            std::locale::global(std::locale::classic());
            std::setlocale(LC_NUMERIC, "C");
        }
        ~NumericLocaleGuard() {
            if (!prevC.empty()) std::setlocale(LC_NUMERIC, prevC.c_str());
            std::locale::global(prevCpp);
        }
    };

    static inline bool ReadUUID(const YAML::Node& n, QuasarEngine::UUID& out)
    {
        if (!n) return false;

        try {
            uint64_t v = n.as<uint64_t>();
            out = QuasarEngine::UUID(v);
            return true;
        }
        catch (...) {}

        if (n.IsScalar()) {
            std::string s = n.Scalar();

            while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            while (!s.empty() && isspace(static_cast<unsigned char>(s.back())))  s.pop_back();

            int base = 10;
            if (s.size() > 2 && (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0))
                base = 16;

            try {
                std::size_t pos = 0;
                uint64_t v = std::stoull(s, &pos, base);
                if (pos == s.size()) {
                    out = QuasarEngine::UUID(v);
                    return true;
                }
            }
            catch (...) {}
        }

        return false;
    }

    static inline YAML::Emitter& operator<<(YAML::Emitter& out, const QuasarEngine::UUID& id)
    {
        out << std::to_string(static_cast<std::uint64_t>(id));
        return out;
    }

    static inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    static inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.r << v.g << v.b << v.a << YAML::EndSeq;
        return out;
    }

    static inline void EmitVec3(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    }

    static inline void EmitVec3Array(YAML::Emitter& out, const std::vector<glm::vec3>& a)
    {
        out << YAML::Flow << YAML::BeginSeq;
        for (const auto& v : a)
            out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        out << YAML::EndSeq;
    }

    static inline void EmitUIntArray(YAML::Emitter& out, const std::vector<uint32_t>& a)
    {
        out << YAML::Flow << YAML::BeginSeq;
        for (auto v : a) out << v;
        out << YAML::EndSeq;
    }

    static inline void EmitFloatArray(YAML::Emitter& out, const std::vector<float>& a)
    {
        out << YAML::Flow << YAML::BeginSeq;
        for (auto v : a) out << v;
        out << YAML::EndSeq;
    }

    static bool AsFloatLocaleSafe(const YAML::Node& n, float& out)
    {
        if (!n) return false;

        try { out = n.as<float>(); return true; }
        catch (...) {}

        if (n.IsScalar()) {
            std::string s = n.Scalar();
            std::replace(s.begin(), s.end(), ',', '.');
            char* pend = nullptr;
            double v = std::strtod(s.c_str(), &pend);
            if (pend && *pend == '\0') { out = static_cast<float>(v); return true; }
        }
        return false;
    }

    static bool ReadVec3LocaleSafe(const YAML::Node& n, glm::vec3& out)
    {
        if (!n || !n.IsSequence() || n.size() != 3) return false;
        float x{}, y{}, z{};
        if (!AsFloatLocaleSafe(n[0], x)) return false;
        if (!AsFloatLocaleSafe(n[1], y)) return false;
        if (!AsFloatLocaleSafe(n[2], z)) return false;
        out = { x, y, z };
        return true;
    }
    static bool ReadVec4LocaleSafe(const YAML::Node& n, glm::vec4& out)
    {
        if (!n || !n.IsSequence() || n.size() != 4) return false;
        float r{}, g{}, b{}, a{};
        if (!AsFloatLocaleSafe(n[0], r)) return false;
        if (!AsFloatLocaleSafe(n[1], g)) return false;
        if (!AsFloatLocaleSafe(n[2], b)) return false;
        if (!AsFloatLocaleSafe(n[3], a)) return false;
        out = { r, g, b, a };
        return true;
    }
    static bool ReadFloatArrayLocaleSafe(const YAML::Node& n, std::vector<float>& out)
    {
        if (!n || !n.IsSequence()) return false;
        out.clear(); out.reserve(n.size());
        for (auto it : n) { float v; if (!AsFloatLocaleSafe(it, v)) return false; out.push_back(v); }
        return true;
    }
    static bool ReadVec3ArrayLocaleSafe(const YAML::Node& n, std::vector<glm::vec3>& out)
    {
        if (!n || !n.IsSequence()) return false;
        out.clear(); out.reserve(n.size());
        for (auto it : n) { glm::vec3 v; if (!ReadVec3LocaleSafe(it, v)) return false; out.push_back(v); }
        return true;
    }
    static bool ReadUIntArray(const YAML::Node& n, std::vector<uint32_t>& out)
    {
        if (!n || !n.IsSequence()) return false;
        out.clear(); out.reserve(n.size());
        for (auto it : n) out.push_back(it.as<uint32_t>());
        return true;
    }

    void SceneSerializer::Serialize(const std::string& filepath) const
    {
        NumericLocaleGuard _locale_guard;

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << m_SceneObject->GetName();
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        for (auto e : m_SceneObject->GetScene().GetAllEntitiesWith<IDComponent>()) {
            Entity entity(e, m_SceneObject->GetScene().GetRegistry());
            if (entity.GetComponent<HierarchyComponent>().m_Parent == UUID::Null())
                SerializeEntity(out, m_SceneObject->GetScene(), entity, m_AssetPath.string());
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath, std::ios::binary);
        if (!fout) throw std::runtime_error("Failed to open file for writing: " + filepath);
        fout << out.c_str();
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        NumericLocaleGuard _locale_guard;

        std::ifstream stream(filepath, std::ios::binary);
        if (!stream) return false;

        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data;
        try {
            data = YAML::Load(strStream.str());
        }
        catch (const YAML::ParserException&) {
            return false;
        }

        if (!data["Scene"] || !data["Entities"]) return false;

        m_SceneObject->SetName(data["Scene"].as<std::string>());
        m_SceneObject->SetPath(filepath);

        auto& scene = m_SceneObject->GetScene();
        const YAML::Node entities = data["Entities"];

        if (!LoadEntities(entities, scene, m_AssetPath.string())) return false;
        if (!SetupHierarchy(entities, scene)) return false;

        return true;
    }

    bool SceneSerializer::LoadEntities(const YAML::Node& entities, Scene& scene, const std::string& assetPath)
    {
        for (const auto& entityNode : entities)
        {
            try {
                std::string name = entityNode["Entity"] ? entityNode["Entity"].as<std::string>() : "Entity";
                UUID uuid = UUID();
                if (entityNode["ID"]) ReadUUID(entityNode["ID"], uuid);

                Entity entity = scene.CreateEntityWithUUID(uuid, name);

                const YAML::Node components = entityNode["Components"];
                if (!components) continue;

                for (const auto& component : components)
                {
                    if (!component.IsMap()) continue;
                    for (const auto& it : component)
                    {
                        const std::string key = it.first.as<std::string>();
                        const YAML::Node  value = it.second;

                        if (key == "TagComponent")
                        {
                            auto& tc = entity.HasComponent<TagComponent>()
                                ? entity.GetComponent<TagComponent>()
                                : entity.AddComponent<TagComponent>();

                            if (const auto n = value["Name"]) tc.Tag = n.as<std::string>();

                            QuasarEngine::TagMask mask = TagMask::None;

                            if (const auto m = value["Mask"])
                            {
                                mask = NodeToMask(m);
                            }
                            if (const auto hx = value["MaskHex"])
                            {
                                const std::string s = hx.as<std::string>();
                                uint64_t u = 0;
                                if (HexToU64(s, u)) mask = static_cast<TagMask>(u);
                            }
                            tc.Mask = mask;
                        }
                        else if (key == "TransformComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<TransformComponent>();
                            glm::vec3 v;
                            if (value["Position"] && ReadVec3LocaleSafe(value["Position"], v)) c.Position = v; else c.Position = glm::vec3(0.f);
                            if (value["Rotation"] && ReadVec3LocaleSafe(value["Rotation"], v)) c.Rotation = v; else c.Rotation = glm::vec3(0.f);
                            if (value["Scale"] && ReadVec3LocaleSafe(value["Scale"], v)) c.Scale = v; else c.Scale = glm::vec3(1.f);
                        }
                        else if (key == "MeshRendererComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<MeshRendererComponent>();
                            if (value["Rendered"]) c.m_Rendered = value["Rendered"].as<bool>();
                        }
                        else if (key == "MeshComponent")
                        {
                            std::string rel = value["Path"] ? value["Path"].as<std::string>() : "";
                            std::string mname = value["Name"] ? value["Name"].as<std::string>() : "";

                            if (rel.empty())
                            {
                                std::cerr << "[MeshComponent] Aucun chemin fourni pour la ressource mesh.\n";
                                continue;
                            }

                            std::filesystem::path fullPath = AssetManager::Instance().ResolvePath(rel);

                            std::string id = std::filesystem::path(rel).generic_string();

                            if (!AssetManager::Instance().isAssetLoaded(id))
                            {
                                AssetToLoad modelAsset;
                                modelAsset.id = id;
                                modelAsset.path = fullPath.generic_string();
                                modelAsset.type = AssetType::MODEL;

                                AssetManager::Instance().loadAsset(modelAsset);
                            }

                            auto& comp = entity.AddOrReplaceComponent<MeshComponent>(mname, nullptr, id);

                            AssetToLoad meshAsset;
                            meshAsset.id = id;
                            meshAsset.path = fullPath.generic_string();
                            meshAsset.type = AssetType::MESH;
                            meshAsset.handle = &comp;

                            AssetManager::Instance().loadAsset(meshAsset);
                        }
                        else if (key == "MaterialComponent")
                        {
                            MaterialSpecification spec;
                            glm::vec4 alb;
                            if (value["albedo"] && ReadVec4LocaleSafe(value["albedo"], alb)) spec.Albedo = alb; else spec.Albedo = glm::vec4(1.f);
                            float f;
                            if (value["metallic"] && AsFloatLocaleSafe(value["metallic"], f)) spec.Metallic = f;   else spec.Metallic = 0.f;
                            if (value["roughness"] && AsFloatLocaleSafe(value["roughness"], f)) spec.Roughness = f;   else spec.Roughness = 0.5f;
                            if (value["ao"] && AsFloatLocaleSafe(value["ao"], f)) spec.AO = f;   else spec.AO = 1.f;

                            auto tryLoadTex = [&](const char* k, std::optional<std::string>& outTex)
                                {
                                    if (!value[k]) return;

                                    std::string texRel = value[k].as<std::string>();

                                    std::string texId = std::filesystem::path(texRel).generic_string();

                                    std::filesystem::path texFull = AssetManager::Instance().ResolvePath(texRel);

                                    outTex = texId;

                                    if (!AssetManager::Instance().isAssetLoaded(texId))
                                    {
                                        TextureSpecification ts = TextureConfigImporter::ImportTextureConfig(texFull.generic_string());

                                        AssetToLoad asset;
                                        asset.id = texId;
                                        asset.path = texFull.generic_string();
                                        asset.type = AssetType::TEXTURE;
                                        asset.spec = ts;

                                        AssetManager::Instance().loadAsset(asset);
                                        //AssetManager::Instance().LoadTextureAsync(asset);
                                    }
                                };

                            tryLoadTex("albedoMap", spec.AlbedoTexture);
                            tryLoadTex("normalMap", spec.NormalTexture);
                            tryLoadTex("metallicMap", spec.MetallicTexture);
                            tryLoadTex("roughnessMap", spec.RoughnessTexture);
                            tryLoadTex("aoMap", spec.AOTexture);

                            entity.AddOrReplaceComponent<MaterialComponent>(spec);
                        }
                        else if (key == "LightComponent")
                        {
                            std::string type = value["lightType"] ? value["lightType"].as<std::string>() : "";
                            if (type == "Directional") {
                                auto& c = entity.AddOrReplaceComponent<LightComponent>(LightComponent::LightType::DIRECTIONAL);
                                glm::vec3 col;
                                if (value["color"] && ReadVec3LocaleSafe(value["color"], col)) c.directional_light.color = col;
                                float f; if (value["power"] && AsFloatLocaleSafe(value["power"], f)) c.directional_light.power = f;
                            }
                            else if (type == "Point") {
                                auto& c = entity.AddOrReplaceComponent<LightComponent>(LightComponent::LightType::POINT);
                                glm::vec3 col;
                                if (value["color"] && ReadVec3LocaleSafe(value["color"], col)) c.point_light.color = col;
                                float f;
                                if (value["attenuation"] && AsFloatLocaleSafe(value["attenuation"], f)) c.point_light.attenuation = f;
                                if (value["power"] && AsFloatLocaleSafe(value["power"], f)) c.point_light.power = f;
                            }
                        }
                        else if (key == "CameraComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<CameraComponent>();
                            c.GetCamera().Init(&entity.GetComponent<TransformComponent>());
                            float f;
                            if (value["fov"] && AsFloatLocaleSafe(value["fov"], f)) c.GetCamera().SetFov(f);
                            if (value["primary"]) {
                                c.Primary = value["primary"].as<bool>();
                            }
                        }
                        else if (key == "RigidBodyComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<RigidBodyComponent>();
                            c.Init();
                            if (value["enableGravity"])       c.enableGravity = value["enableGravity"].as<bool>();
                            if (value["bodyType"])            c.bodyTypeString = value["bodyType"].as<std::string>();
                            if (value["linearAxisFactor_X"])  c.m_LinearAxisFactorX = value["linearAxisFactor_X"].as<bool>();
                            if (value["linearAxisFactor_Y"])  c.m_LinearAxisFactorY = value["linearAxisFactor_Y"].as<bool>();
                            if (value["linearAxisFactor_Z"])  c.m_LinearAxisFactorZ = value["linearAxisFactor_Z"].as<bool>();
                            if (value["angularAxisFactor_X"]) c.m_AngularAxisFactorX = value["angularAxisFactor_X"].as<bool>();
                            if (value["angularAxisFactor_Y"]) c.m_AngularAxisFactorY = value["angularAxisFactor_Y"].as<bool>();
                            if (value["angularAxisFactor_Z"]) c.m_AngularAxisFactorZ = value["angularAxisFactor_Z"].as<bool>();
                            float f;
                            if (value["linearDamping"] && AsFloatLocaleSafe(value["linearDamping"], f)) c.linearDamping = f;
                            if (value["angularDamping"] && AsFloatLocaleSafe(value["angularDamping"], f)) c.angularDamping = f;

                            c.UpdateEnableGravity();
                            c.UpdateBodyType();
                            c.UpdateLinearAxisFactor();
                            c.UpdateAngularAxisFactor();
                            c.UpdateDamping();
                        }
                        else if (key == "BoxColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<BoxColliderComponent>();
                            c.Init();
                            float f;
                            if (value["mass"] && AsFloatLocaleSafe(value["mass"], f)) c.mass = f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();
                            glm::vec3 v;
                            if (value["size"] && ReadVec3LocaleSafe(value["size"], v)) c.m_Size = v;
                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "SphereColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<SphereColliderComponent>();
                            c.Init();
                            float f;
                            if (value["mass"] && AsFloatLocaleSafe(value["mass"], f)) c.mass = f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["radius"] && AsFloatLocaleSafe(value["radius"], f)) c.m_Radius = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();
                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "CapsuleColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<CapsuleColliderComponent>();
                            c.Init();
                            float f;
                            if (value["mass"] && AsFloatLocaleSafe(value["mass"], f)) c.mass = f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["radius"] && AsFloatLocaleSafe(value["radius"], f)) c.m_Radius = f;
                            if (value["height"] && AsFloatLocaleSafe(value["height"], f)) c.m_Height = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();
                            if (value["axis"]) {
                                const std::string ax = value["axis"].as<std::string>();
                                if (ax == "X") c.m_Axis = CapsuleColliderComponent::Axis::X;
                                else if (ax == "Z") c.m_Axis = CapsuleColliderComponent::Axis::Z;
                                else                c.m_Axis = CapsuleColliderComponent::Axis::Y;
                            }
                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "PlaneColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<PlaneColliderComponent>();
                            c.Init();
                            float f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["useEntityOrientation"]) c.m_UseEntityOrientation = value["useEntityOrientation"].as<bool>();
                            glm::vec3 v;
                            if (value["normal"] && ReadVec3LocaleSafe(value["normal"], v)) c.m_Normal = v;
                            if (value["distance"] && AsFloatLocaleSafe(value["distance"], f)) c.m_Distance = f;
                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "ConvexMeshColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<ConvexMeshColliderComponent>();
                            c.Init();
                            float f;
                            if (value["mass"] && AsFloatLocaleSafe(value["mass"], f)) c.mass = f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();
                            if (value["points"]) {
                                std::vector<glm::vec3> pts;
                                if (ReadVec3ArrayLocaleSafe(value["points"], pts)) c.SetPoints(pts);
                            }
                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "TriangleMeshColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<TriangleMeshColliderComponent>();
                            c.Init();
                            float f;
                            if (value["mass"] && AsFloatLocaleSafe(value["mass"], f)) c.mass = f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();

                            std::vector<glm::vec3> verts;
                            std::vector<uint32_t>  indices;
                            if (value["vertices"]) ReadVec3ArrayLocaleSafe(value["vertices"], verts);
                            if (value["indices"])  ReadUIntArray(value["indices"], indices);
                            if (!verts.empty() && !indices.empty()) c.SetMesh(verts, indices);

                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "HeightfieldColliderComponent")
                        {
                            auto& c = entity.AddOrReplaceComponent<HeightfieldColliderComponent>();
                            c.Init();
                            float f;
                            if (value["friction"] && AsFloatLocaleSafe(value["friction"], f)) c.friction = f;
                            if (value["bounciness"] && AsFloatLocaleSafe(value["bounciness"], f)) c.bounciness = f;
                            if (value["useEntityScale"]) c.m_UseEntityScale = value["useEntityScale"].as<bool>();

                            uint32_t rows = value["rows"] ? value["rows"].as<uint32_t>() : 0u;
                            uint32_t cols = value["cols"] ? value["cols"].as<uint32_t>() : 0u;
                            float csX = 1.f, csZ = 1.f;
                            if (value["cellSizeX"]) AsFloatLocaleSafe(value["cellSizeX"], csX);
                            if (value["cellSizeZ"]) AsFloatLocaleSafe(value["cellSizeZ"], csZ);

                            std::vector<float> heights;
                            if (value["heights"]) ReadFloatArrayLocaleSafe(value["heights"], heights);
                            if (rows >= 2 && cols >= 2 && heights.size() == size_t(rows) * size_t(cols))
                                c.SetHeightData(rows, cols, heights, csX, csZ);

                            c.UpdateColliderMaterial();
                            c.UpdateColliderSize();
                        }
                        else if (key == "ScriptComponent")
                        {
                            auto& sc = entity.AddOrReplaceComponent<ScriptComponent>();
                            if (value["scriptPath"]) {
                                sc.scriptPath = value["scriptPath"].as<std::string>();
                            }

							sc.Initialize();
                        }
                    }
                }
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception lors du chargement de l'entité : " << ex.what() << std::endl;
                continue;
            }
            catch (...) {
                std::cerr << "Exception inconnue lors du chargement de l'entité" << std::endl;
                continue;
            }
        }
        return true;
    }

    bool SceneSerializer::SetupHierarchy(const YAML::Node& entities, Scene& scene)
    {
        for (const auto& entityNode : entities)
        {
            if (!entityNode["ID"] || !entityNode["ParentID"]) continue;

            UUID uuid = UUID::Null();
            if (entityNode["ID"])
                ReadUUID(entityNode["ID"], uuid);

            UUID parentUUID = UUID::Null();
            if (entityNode["ParentID"])
                ReadUUID(entityNode["ParentID"], parentUUID);

            if (parentUUID == UUID::Null()) continue;

            auto entityOpt = scene.GetEntityByUUID(uuid);
            auto parentOpt = scene.GetEntityByUUID(parentUUID);

            if (entityOpt && parentOpt) {
                Entity& entity = *entityOpt;
                Entity& parent = *parentOpt;
                entity.GetComponent<HierarchyComponent>().m_Parent = parent.GetUUID();
                parent.GetComponent<HierarchyComponent>().AddChild(parent.GetUUID(), entity.GetUUID());
            }
        }
        return true;
    }

    void SceneSerializer::SerializeEntity(YAML::Emitter& out, Scene& scene, Entity entity, const std::string& assetPath) const
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << entity.GetName();
        out << YAML::Key << "ID" << YAML::Value << entity.GetUUID();

        const UUID parentID = (entity.GetComponent<HierarchyComponent>().m_Parent != UUID::Null())
            ? entity.GetComponent<HierarchyComponent>().m_Parent
            : UUID::Null();
        out << YAML::Key << "ParentID" << YAML::Value << parentID;

        out << YAML::Key << "Components" << YAML::Value << YAML::BeginSeq;

        if (entity.HasComponent<TagComponent>())
        {
            const auto& tc = entity.GetComponent<TagComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "TagComponent" << YAML::Value << YAML::BeginMap;

            out << YAML::Key << "Name" << YAML::Value << tc.Tag;

            {
                const auto names = MaskToNames(tc.Mask);
                out << YAML::Key << "Mask" << YAML::Value << YAML::BeginSeq;
                for (const auto& n : names) out << n;
                out << YAML::EndSeq;
            }

            out << YAML::Key << "MaskHex" << YAML::Value << U64ToHex(static_cast<uint64_t>(tc.Mask));

            out << YAML::EndMap;
            out << YAML::EndMap;
        }

        auto BeginComp = [&](const char* name) {
            out << YAML::BeginMap;
            out << YAML::Key << name << YAML::Value << YAML::BeginMap;
            };
        auto EndComp = [&]() {
            out << YAML::EndMap;
            out << YAML::EndMap;
            };

        if (entity.HasComponent<TransformComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
            const auto& tc = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "Position" << YAML::Value << tc.Position;
            out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<MaterialComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "MaterialComponent" << YAML::Value << YAML::BeginMap;
            auto& mc = entity.GetComponent<MaterialComponent>();
            const auto& spec = mc.GetMaterial().GetSpecification();

            out << YAML::Key << "albedo" << YAML::Value << spec.Albedo;
            out << YAML::Key << "metallic" << YAML::Value << spec.Metallic;
            out << YAML::Key << "roughness" << YAML::Value << spec.Roughness;
            out << YAML::Key << "ao" << YAML::Value << spec.AO;

            auto putTex = [&](TextureType tt, const char* key) {
                if (mc.GetMaterial().HasTexture(tt)) {
                    auto rel = Utils::getRelativePath(*mc.GetMaterial().GetTexturePath(tt), assetPath);
                    out << YAML::Key << key << YAML::Value << (rel ? *rel : "");
                }
                };
            putTex(TextureType::Albedo, "albedoMap");
            putTex(TextureType::Normal, "normalMap");
            putTex(TextureType::Metallic, "metallicMap");
            putTex(TextureType::Roughness, "roughnessMap");
            putTex(TextureType::AO, "aoMap");

            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<MeshRendererComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "MeshRendererComponent" << YAML::Value << YAML::BeginMap;
            const auto& mrc = entity.GetComponent<MeshRendererComponent>();
            out << YAML::Key << "Rendered" << YAML::Value << mrc.m_Rendered;
            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<MeshComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
            auto& mc = entity.GetComponent<MeshComponent>();
            auto rel = Utils::getRelativePath(mc.GetModelPath(), assetPath);
            out << YAML::Key << "Path" << YAML::Value << (rel ? *rel : "");
            out << YAML::Key << "Name" << YAML::Value << mc.GetName();
            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<LightComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "LightComponent" << YAML::Value << YAML::BeginMap;
            const auto& lc = entity.GetComponent<LightComponent>();
            if (lc.lightType == LightComponent::LightType::DIRECTIONAL) {
                out << YAML::Key << "lightType" << YAML::Value << "Directional";
                out << YAML::Key << "color" << YAML::Value << lc.directional_light.color;
                out << YAML::Key << "power" << YAML::Value << lc.directional_light.power;
            }
            else if (lc.lightType == LightComponent::LightType::POINT) {
                out << YAML::Key << "lightType" << YAML::Value << "Point";
                out << YAML::Key << "color" << YAML::Value << lc.point_light.color;
                out << YAML::Key << "attenuation" << YAML::Value << lc.point_light.attenuation;
                out << YAML::Key << "power" << YAML::Value << lc.point_light.power;
            }
            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<CameraComponent>()) {
            out << YAML::BeginMap;
            out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
            auto& cc = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "fov" << YAML::Value << cc.GetCamera().GetFov();
            out << YAML::Key << "primary" << YAML::Value << cc.Primary;
            out << YAML::EndMap << YAML::EndMap;
        }

        if (entity.HasComponent<RigidBodyComponent>()) {
            const auto& c = entity.GetComponent<RigidBodyComponent>();
            BeginComp("RigidBodyComponent");
            out << YAML::Key << "enableGravity" << YAML::Value << c.enableGravity;
            out << YAML::Key << "bodyType" << YAML::Value << c.bodyTypeString;
            out << YAML::Key << "linearAxisFactor_X" << YAML::Value << c.m_LinearAxisFactorX;
            out << YAML::Key << "linearAxisFactor_Y" << YAML::Value << c.m_LinearAxisFactorY;
            out << YAML::Key << "linearAxisFactor_Z" << YAML::Value << c.m_LinearAxisFactorZ;
            out << YAML::Key << "angularAxisFactor_X" << YAML::Value << c.m_AngularAxisFactorX;
            out << YAML::Key << "angularAxisFactor_Y" << YAML::Value << c.m_AngularAxisFactorY;
            out << YAML::Key << "angularAxisFactor_Z" << YAML::Value << c.m_AngularAxisFactorZ;
            out << YAML::Key << "linearDamping" << YAML::Value << c.linearDamping;
            out << YAML::Key << "angularDamping" << YAML::Value << c.angularDamping;
            EndComp();
        }

        if (entity.HasComponent<BoxColliderComponent>()) {
            const auto& c = entity.GetComponent<BoxColliderComponent>();
            BeginComp("BoxColliderComponent");
            out << YAML::Key << "mass" << YAML::Value << c.mass;
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            out << YAML::Key << "size" << YAML::Value; EmitVec3(out, c.m_Size);
            EndComp();
        }

        if (entity.HasComponent<SphereColliderComponent>()) {
            const auto& c = entity.GetComponent<SphereColliderComponent>();
            BeginComp("SphereColliderComponent");
            out << YAML::Key << "mass" << YAML::Value << c.mass;
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "radius" << YAML::Value << c.m_Radius;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            EndComp();
        }

        if (entity.HasComponent<CapsuleColliderComponent>()) {
            const auto& c = entity.GetComponent<CapsuleColliderComponent>();
            const char* axisStr = (c.m_Axis == CapsuleColliderComponent::Axis::X) ? "X"
                : (c.m_Axis == CapsuleColliderComponent::Axis::Y) ? "Y" : "Z";
            BeginComp("CapsuleColliderComponent");
            out << YAML::Key << "mass" << YAML::Value << c.mass;
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "radius" << YAML::Value << c.m_Radius;
            out << YAML::Key << "height" << YAML::Value << c.m_Height;
            out << YAML::Key << "axis" << YAML::Value << axisStr;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            EndComp();
        }

        if (entity.HasComponent<PlaneColliderComponent>()) {
            const auto& c = entity.GetComponent<PlaneColliderComponent>();
            BeginComp("PlaneColliderComponent");
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "useEntityOrientation" << YAML::Value << c.m_UseEntityOrientation;
            out << YAML::Key << "normal" << YAML::Value; EmitVec3(out, c.m_Normal);
            out << YAML::Key << "distance" << YAML::Value << c.m_Distance;
            EndComp();
        }

        if (entity.HasComponent<ConvexMeshColliderComponent>()) {
            const auto& c = entity.GetComponent<ConvexMeshColliderComponent>();
            BeginComp("ConvexMeshColliderComponent");
            out << YAML::Key << "mass" << YAML::Value << c.mass;
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            out << YAML::Key << "points" << YAML::Value; EmitVec3Array(out, c.GetPoints());
            EndComp();
        }

        if (entity.HasComponent<TriangleMeshColliderComponent>()) {
            const auto& c = entity.GetComponent<TriangleMeshColliderComponent>();
            BeginComp("TriangleMeshColliderComponent");
            out << YAML::Key << "mass" << YAML::Value << c.mass;
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            out << YAML::Key << "vertices" << YAML::Value; EmitVec3Array(out, c.GetVertices());
            out << YAML::Key << "indices" << YAML::Value; EmitUIntArray(out, c.GetIndices());
            EndComp();
        }

        if (entity.HasComponent<HeightfieldColliderComponent>()) {
            const auto& c = entity.GetComponent<HeightfieldColliderComponent>();
            BeginComp("HeightfieldColliderComponent");
            out << YAML::Key << "friction" << YAML::Value << c.friction;
            out << YAML::Key << "bounciness" << YAML::Value << c.bounciness;
            out << YAML::Key << "useEntityScale" << YAML::Value << c.m_UseEntityScale;
            out << YAML::Key << "rows" << YAML::Value << c.GetRows();
            out << YAML::Key << "cols" << YAML::Value << c.GetCols();
            out << YAML::Key << "cellSizeX" << YAML::Value << c.GetCellSizeX();
            out << YAML::Key << "cellSizeZ" << YAML::Value << c.GetCellSizeZ();
            out << YAML::Key << "heights" << YAML::Value; EmitFloatArray(out, c.GetHeights());
            EndComp();
        }

        if (entity.HasComponent<ScriptComponent>()) {
            const auto& sc = entity.GetComponent<ScriptComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "ScriptComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "scriptPath" << YAML::Value << sc.scriptPath;
            out << YAML::EndMap << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        const auto& childrens = entity.GetComponent<HierarchyComponent>().m_Childrens;
        for (auto childUUID : childrens) {
            if (auto childEntity = scene.GetEntityByUUID(childUUID)) {
                SerializeEntity(out, scene, childEntity.value_or({}), assetPath);
            }
        }
    }
}