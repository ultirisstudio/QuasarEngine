#include "qepch.h"

#include <filesystem>

#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Asset/AssetManager.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <assimp/pbrmaterial.h>
#include <glm/gtc/type_ptr.hpp>

namespace QuasarEngine
{
    inline glm::mat4 AiToGlm(const aiMatrix4x4& m)
    {
        return glm::transpose(glm::make_mat4(&m.a1));
    }

    Mesh* Model::loadMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
    {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D v = mesh->mVertices[i];
            glm::vec4 pos = glm::vec4(v.x, v.y, v.z, 1.0f);
            pos = transform * pos;
            vertices.insert(vertices.end(), { pos.x, pos.y, pos.z });

            if (mesh->HasNormals())
            {
                aiVector3D n = mesh->mNormals[i];
                glm::vec4 norm = glm::vec4(n.x, n.y, n.z, 0.0f);
                norm = transform * norm;
                vertices.insert(vertices.end(), { norm.x, norm.y, norm.z });
            }
            else
            {
                vertices.insert(vertices.end(), { 0.0f , 0.0f, 0.0f });
            }

            if (mesh->mTextureCoords[0])
            {
                vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x , mesh->mTextureCoords[0][i].y });
            }
            else
            {
                vertices.insert(vertices.end(), { 0.0f , 0.0f });
            }

            /*if (mesh->HasTangentsAndBitangents())
            {
                aiVector3D t = mesh->mTangents[i];
                aiVector3D b = mesh->mBitangents[i];

                vertices.insert(vertices.end(), { t.x, t.y, t.z });
                vertices.insert(vertices.end(), { b.x, b.y, b.z });
            }*/
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        /////////////////////////////// MATERIAL ///////////////////////////////

        MaterialSpecification mat;

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            auto projectRoot = AssetManager::Instance().getAssetPath();
            auto toAbs = [&](const std::filesystem::path& p) -> std::filesystem::path {
                if (p.is_absolute()) return p;
                std::error_code ec;
                auto cand = std::filesystem::weakly_canonical(m_SourceDir / p, ec);
                return ec ? (m_SourceDir / p) : cand;
                };
            auto toIdIfExists = [&](const std::filesystem::path& abs) -> std::optional<std::string> {
                std::error_code ec;
                if (!std::filesystem::exists(abs, ec)) return std::nullopt;
                auto rel = std::filesystem::relative(abs, projectRoot, ec);
                if (ec || rel.empty()) return std::nullopt;
                return rel.generic_string();
                };
            auto isEmbedded = [](const aiString& s) {
                return !s.C_Str() ? false : (s.C_Str()[0] == '*');
                };

            auto loadTextureType = [&](aiTextureType type, std::optional<std::string>& target) {
                if (material->GetTextureCount(type) == 0) return;
                aiString str;
                material->GetTexture(type, 0, &str);
                if (isEmbedded(str)) return;

                std::filesystem::path fromAssimp = str.C_Str();
                auto abs = toAbs(fromAssimp);
                if (auto id = toIdIfExists(abs)) target = *id;
                };

            loadTextureType(aiTextureType_DIFFUSE, mat.AlbedoTexture);
            loadTextureType(aiTextureType_NORMALS, mat.NormalTexture);
            if (!mat.NormalTexture) loadTextureType(aiTextureType_HEIGHT, mat.NormalTexture);

            if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) {
                for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_UNKNOWN); ++i) {
                    aiString path;
                    material->GetTexture(aiTextureType_UNKNOWN, i, &path);
                    if (isEmbedded(path)) continue;

                    std::string s = path.C_Str();
                    std::filesystem::path abs = toAbs(s);
                    if (!std::filesystem::exists(abs)) continue;

                    std::string lower = s;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                    if (lower.find("metal") != std::string::npos) {
                        //if (auto id = toIdIfExists(abs)) mat.MetallicTexture = *id;
                    }
                    if (lower.find("rough") != std::string::npos) {
                        //if (auto id = toIdIfExists(abs)) mat.RoughnessTexture = *id;
                    }
                }
            }

            loadTextureType(aiTextureType_LIGHTMAP, mat.AOTexture);

            aiColor3D color(1.f, 1.f, 1.f);
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                mat.Albedo = glm::vec4(color.r, color.g, color.b, 1.0f);
            else
                mat.Albedo = glm::vec4(1.0f);

            float metallic = 0.0f, roughness = 1.0f;
            //material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metallic);
            //material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughness);
            mat.Metallic = metallic;
            mat.Roughness = roughness;
        }

        return new Mesh(vertices, indices, {}, DrawMode::TRIANGLES, mat);
    }

    void Model::loadNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
    {
        glm::mat4 nodeTransform = AiToGlm(node->mTransformation);
        glm::mat4 globalTransform = parentTransform * nodeTransform;

        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            m_meshesMap[mesh->mName.C_Str()] = loadMesh(mesh, scene, globalTransform);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
            loadNode(node->mChildren[i], scene, globalTransform);
    }

    Model::Model(const std::string& path) : m_Name("Unamed")
    {
        m_SourcePath = std::filesystem::path(path);
        m_SourceDir = m_SourcePath.parent_path();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_OptimizeMeshes |
            aiProcess_GenUVCoords | aiProcess_ImproveCacheLocality);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "Failed to load model at " << path << " : " << importer.GetErrorString() << std::endl;
            return;
        }

        m_Name = scene->mRootNode->mName.C_Str();
        loadNode(scene->mRootNode, scene, glm::mat4(1.0f));
    }

	Model::Model(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices) : m_Name(name)
	{
		m_meshesMap[m_Name] = new Mesh(vertices, indices, {});
	}

	Model::~Model()
	{
		for (auto& [name, mesh] : m_meshesMap)
			delete mesh;

		m_meshesMap.clear();
	}

	void Model::draw() const
	{
		for (auto& [name, mesh] : m_meshesMap)
			mesh->draw();
	}

	std::shared_ptr<Model> Model::CreateModel(const std::string& path)
	{
		return std::make_shared<Model>(path);
	}

	std::shared_ptr<Model> Model::CreateModel(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices)
	{
		return std::make_shared<Model>(name, vertices, indices);
	}

	Mesh* Model::GetMesh(const std::string& name)
	{
		return m_meshesMap[name];
	}

	std::unordered_map<std::string, Mesh*>& Model::GetMeshes()
	{
		return m_meshesMap;
	}
}
