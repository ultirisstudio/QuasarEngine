#pragma once

#include <filesystem>
#include <string>

namespace YAML { class Emitter; class Node; }

namespace QuasarEngine
{
    class SceneObject;
    class Scene;
    class Entity;

    class SceneSerializer
    {
    public:
        SceneSerializer(SceneObject& sceneObject, std::filesystem::path assetPath) noexcept
            : m_SceneObject(&sceneObject), m_AssetPath(std::move(assetPath)) {
        }

        void Serialize(const std::string& filepath) const;
        [[nodiscard]] bool Deserialize(const std::string& filepath);

        const std::filesystem::path& GetAssetPath() const noexcept { return m_AssetPath; }
        void SetAssetPath(std::filesystem::path p) noexcept { m_AssetPath = std::move(p); }

    private:
        void SerializeEntity(YAML::Emitter& out, Entity entity, const std::string& assetPath) const;
        bool LoadEntities(const YAML::Node& entities, Scene& scene, const std::string& assetPath);
        bool SetupHierarchy(const YAML::Node& entities, Scene& scene);

    private:
        SceneObject* m_SceneObject = nullptr;
        std::filesystem::path m_AssetPath;
    };
}