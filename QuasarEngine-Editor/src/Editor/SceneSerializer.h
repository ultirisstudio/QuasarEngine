#pragma once

#include "SceneObject.h"

#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace QuasarEngine
{
    class SceneSerializer {
    public:
        SceneSerializer(SceneObject& sceneObject, std::filesystem::path assetPath)
            : m_SceneObject(&sceneObject), m_AssetPath(std::move(assetPath)) {
        }

        void Serialize(const std::string& filepath) const;
        bool Deserialize(const std::string& filepath);

    private:
        void SerializeEntity(YAML::Emitter& out, Entity entity, const std::string& assetPath) const;
        bool LoadEntities(const YAML::Node& entities, Scene& scene, const std::string& assetPath);
        bool SetupHierarchy(const YAML::Node& entities, Scene& scene);

        SceneObject* m_SceneObject;
        std::filesystem::path m_AssetPath;
    };
}