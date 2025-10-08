#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <functional>

#include <glm/glm.hpp>
#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Asset/Asset.h>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace QuasarEngine
{
    constexpr int QE_MAX_BONE_INFLUENCE = 4;

    struct BoneInfo {
        int id = -1;
        glm::mat4 offset{ 1.f };
    };

    struct MeshInstance {
        std::string name;
        std::shared_ptr<Mesh> mesh;
        MaterialSpecification material;
        bool skinned = false;
    };

    struct ModelNode {
        std::string name;
        glm::mat4   localTransform{ 1.f };
        std::vector<MeshInstance> meshes;
        std::vector<std::unique_ptr<ModelNode>> children;
    };

    class Model : public Asset {
    public:
        explicit Model(const std::string& path);
        Model(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices);
        ~Model();

        static std::shared_ptr<Model> CreateModel(const std::string& path);
        static std::shared_ptr<Model> CreateModel(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices);

        const ModelNode* GetRoot() const { return m_root.get(); }
        ModelNode* GetRoot() { return m_root.get(); }

        void ForEachInstance(const std::function<void(const MeshInstance&, const glm::mat4& nodeLocal, const std::string& nodePath)>& fn) const;

        const std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_boneInfoMap; }
        int GetBoneCount() const { return m_boneCount; }

        std::shared_ptr<Mesh> FindMeshByInstanceName(const std::string& instanceName) const;
        std::shared_ptr<Mesh> FindMeshByPathAndName(const std::string& nodePath, const std::string& instanceName) const;

        const std::string& GetName() const { return m_Name; }
        const std::filesystem::path& GetSourcePath() const { return m_SourcePath; }

        const glm::mat4& GetGlobalInverse() const { return m_GlobalInverse; }

        static AssetType GetStaticType() { return AssetType::MODEL; }
        AssetType GetType() override { return GetStaticType(); }

    private:
        void loadFromFile(const std::string& path);
        std::unique_ptr<ModelNode> buildNode(const aiNode* src, const aiScene* scene);

        struct BuiltGeometry {
            std::vector<float>         vertices;
            std::vector<unsigned int>  indices;
            std::vector<int>           boneIDs;
            std::vector<float>         boneWeights;
            bool                        skinned = false;
        };
        BuiltGeometry buildMeshGeometry(const aiMesh* mesh);
        MaterialSpecification loadMaterial(const aiMaterial* material, const std::filesystem::path& modelDir);

        static std::string MakeUniqueChildName(const std::string& base, int index);

    private:
        std::unique_ptr<ModelNode> m_root;
        std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshLibrary;

        std::unordered_map<std::string, BoneInfo> m_boneInfoMap;
        int m_boneCount = 0;

        glm::mat4 m_GlobalInverse{ 1.0f };

        std::string m_Name = "Unnamed";
        std::filesystem::path m_SourcePath;
        std::filesystem::path m_SourceDir;
    };
}