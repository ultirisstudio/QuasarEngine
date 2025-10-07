#pragma once

#include <unordered_map>
#include <filesystem>

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Asset/Asset.h>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace QuasarEngine
{
	class Model : public Asset
	{
	private:
		std::unordered_map<std::string, Mesh*> m_meshesMap;

		std::string m_Name;

		std::filesystem::path m_SourcePath;
		std::filesystem::path m_SourceDir;

		Mesh* loadMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
		void loadNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
	public:
		Model(const std::string& path);
		Model(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices);
		~Model();

		static std::shared_ptr<Model> CreateModel(const std::string& path);
		static std::shared_ptr<Model> CreateModel(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices);

		Mesh* GetMesh(const std::string& name);
		std::unordered_map<std::string, Mesh*>& GetMeshes();

		void draw() const;

		static AssetType GetStaticType() { return AssetType::MODEL; }
		virtual AssetType GetType() override { return GetStaticType(); }
	};
}