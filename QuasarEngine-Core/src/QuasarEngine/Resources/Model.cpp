#include "qepch.h"

#include <algorithm>
#include <stack>
#include <iostream>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/matrix4x4.h>

#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    namespace {
        inline glm::mat4 AiToGlmLocal(const aiMatrix4x4& m)
        {
            glm::mat4 r;
            r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
            r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
            r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
            r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
            return r;
        }

        inline bool IsEmbedded(const aiString& s) { return s.C_Str() && s.C_Str()[0] == '*'; }
    }

    MaterialSpecification Model::loadMaterial(const aiMaterial* material, const std::filesystem::path& modelDir)
    {
        MaterialSpecification mat;
        auto projectRoot = AssetManager::Instance().getAssetPath();

        auto toAbs = [&](const std::filesystem::path& p) -> std::filesystem::path {
            if (p.is_absolute()) return p;
            std::error_code ec; auto cand = std::filesystem::weakly_canonical(modelDir / p, ec);
            return ec ? (modelDir / p) : cand;
            };
        auto toIdIfExists = [&](const std::filesystem::path& abs) -> std::optional<std::string> {
            std::error_code ec; if (!std::filesystem::exists(abs, ec)) return std::nullopt;
            auto rel = std::filesystem::relative(abs, projectRoot, ec);
            if (ec || rel.empty()) return std::nullopt; return rel.generic_string();
            };
        auto loadTextureType = [&](aiTextureType type, std::optional<std::string>& target) {
            if (material->GetTextureCount(type) == 0) return; aiString s; material->GetTexture(type, 0, &s);
            if (IsEmbedded(s)) return; auto abs = toAbs(std::filesystem::path(s.C_Str()));
            if (auto id = toIdIfExists(abs)) target = *id;
            };

        loadTextureType(aiTextureType_DIFFUSE, mat.AlbedoTexture);
        loadTextureType(aiTextureType_NORMALS, mat.NormalTexture);
        if (!mat.NormalTexture) loadTextureType(aiTextureType_HEIGHT, mat.NormalTexture);
        loadTextureType(aiTextureType_LIGHTMAP, mat.AOTexture);

        aiColor3D color(1.f, 1.f, 1.f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            mat.Albedo = glm::vec4(color.r, color.g, color.b, 1.0f);
        else mat.Albedo = glm::vec4(1.0f);

        float metallic = 0.0f, roughness = 1.0f;
        mat.Metallic = metallic; mat.Roughness = roughness;
        return mat;
    }

    std::string Model::MakeUniqueChildName(const std::string& base, int index)
    {
        return base + "_" + std::to_string(index);
    }

    Model::BuiltGeometry Model::buildMeshGeometry(const aiMesh* mesh)
    {
        BuiltGeometry out;
        const bool hasNormals = mesh->HasNormals();
        const bool hasUV0 = mesh->mTextureCoords[0] != nullptr;

        out.vertices.reserve(static_cast<size_t>(mesh->mNumVertices) * 8);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            const aiVector3D& v = mesh->mVertices[i];
            out.vertices.push_back(v.x); out.vertices.push_back(v.y); out.vertices.push_back(v.z);
            if (hasNormals) {
                const aiVector3D& n = mesh->mNormals[i];
                out.vertices.push_back(n.x); out.vertices.push_back(n.y); out.vertices.push_back(n.z);
            }
            else {
                out.vertices.insert(out.vertices.end(), { 0.f,0.f,0.f });
            }
            if (hasUV0) {
                out.vertices.push_back(mesh->mTextureCoords[0][i].x);
                out.vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else {
                out.vertices.insert(out.vertices.end(), { 0.f,0.f });
            }
        }

        out.indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3u);
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& f = mesh->mFaces[i];
            for (unsigned int j = 0; j < f.mNumIndices; ++j)
                out.indices.push_back(f.mIndices[j]);
        }

        if (mesh->mNumBones > 0)
        {
			std::cout << "Mesh " << mesh->mName.C_Str() << " has " << mesh->mNumBones << " bones.\n";
            out.skinned = true;
            const size_t vcount = mesh->mNumVertices;
            out.boneIDs.assign(vcount * QE_MAX_BONE_INFLUENCE, -1);
            out.boneWeights.assign(vcount * QE_MAX_BONE_INFLUENCE, 0.0f);

            auto setInfluence = [&](unsigned vtx, int boneID, float w) {
                const size_t base = static_cast<size_t>(vtx) * QE_MAX_BONE_INFLUENCE;
                for (int k = 0; k < QE_MAX_BONE_INFLUENCE; ++k) {
                    if (out.boneIDs[base + k] < 0) { out.boneIDs[base + k] = boneID; out.boneWeights[base + k] = w; return; }
                }
                };

            for (unsigned b = 0; b < mesh->mNumBones; ++b)
            {
                const aiBone* aibone = mesh->mBones[b];
                const std::string boneName = aibone->mName.C_Str();
                int boneID;
                if (auto it = m_boneInfoMap.find(boneName); it == m_boneInfoMap.end()) {
                    BoneInfo info; info.id = m_boneCount++; info.offset = AiToGlmLocal(aibone->mOffsetMatrix);
                    m_boneInfoMap.emplace(boneName, info); boneID = info.id;
                }
                else boneID = it->second.id;

                for (unsigned w = 0; w < aibone->mNumWeights; ++w)
                {
                    const auto& vw = aibone->mWeights[w];
                    if (vw.mVertexId < mesh->mNumVertices)
                        setInfluence(vw.mVertexId, boneID, vw.mWeight);
                }
            }
        }
        return out;
    }

    std::unique_ptr<ModelNode> Model::buildNode(const aiNode* src, const aiScene* scene)
    {
        auto node = std::make_unique<ModelNode>();
        node->name = src->mName.C_Str();
        node->localTransform = AiToGlmLocal(src->mTransformation);

        for (unsigned i = 0; i < src->mNumMeshes; ++i)
        {
            const aiMesh* aimesh = scene->mMeshes[src->mMeshes[i]];
            BuiltGeometry geom = buildMeshGeometry(aimesh);

            const std::string key = node->name + ":" + aimesh->mName.C_Str() + ":" + std::to_string(i);
            std::shared_ptr<Mesh> meshAsset;
            if (auto it = m_meshLibrary.find(key); it == m_meshLibrary.end())
            {
                meshAsset = std::make_shared<Mesh>(
                    geom.vertices,
                    geom.indices,
                    std::nullopt,
                    DrawMode::TRIANGLES,
                    MaterialSpecification{}
                );

                if (geom.skinned) {
                    meshAsset->SetSkinning(geom.boneIDs, geom.boneWeights, QE_MAX_BONE_INFLUENCE);
                }
                m_meshLibrary.emplace(key, meshAsset);
            }
            else meshAsset = it->second;

            MeshInstance inst;
            inst.name = aimesh->mName.length ? aimesh->mName.C_Str() : MakeUniqueChildName("mesh", static_cast<int>(i));
            inst.mesh = meshAsset;
            inst.skinned = geom.skinned;

            MaterialSpecification mat;
            if (aimesh->mMaterialIndex >= 0)
                mat = loadMaterial(scene->mMaterials[aimesh->mMaterialIndex], m_SourceDir);
            inst.material = std::move(mat);

            node->meshes.push_back(std::move(inst));
        }

        node->children.reserve(src->mNumChildren);
        for (unsigned c = 0; c < src->mNumChildren; ++c)
            node->children.push_back(buildNode(src->mChildren[c], scene));

        return node;
    }

    Model::Model(const std::string& path) { loadFromFile(path); }

    Model::Model(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices)
        : m_Name(std::move(name))
    {
        auto root = std::make_unique<ModelNode>();
        root->name = m_Name;
        auto mesh = std::make_shared<Mesh>(vertices, indices, std::nullopt);
        MeshInstance inst; inst.name = m_Name; inst.mesh = mesh; root->meshes.push_back(std::move(inst));
        m_meshLibrary.emplace(m_Name, std::move(mesh));
        m_root = std::move(root);
    }

    Model::~Model() = default;

    void Model::loadFromFile(const std::string& path)
    {
        m_SourcePath = std::filesystem::path(path);
        m_SourceDir = m_SourcePath.parent_path();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenUVCoords |
            aiProcess_ImproveCacheLocality |
            aiProcess_LimitBoneWeights |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |
            aiProcess_SortByPType
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
        {
            std::cout << "Failed to load model at " << path << " : " << importer.GetErrorString() << std::endl;
            m_root = std::make_unique<ModelNode>(); m_root->name = "<invalid>"; m_Name = m_root->name; return;
        }

        m_Name = scene->mRootNode->mName.C_Str();
        m_root = buildNode(scene->mRootNode, scene);
    }

    void Model::ForEachInstance(const std::function<void(const MeshInstance&, const glm::mat4&, const std::string&)>& fn) const
    {
        if (!m_root) return;
        struct StackItem { const ModelNode* node; std::string path; };
        std::stack<StackItem> st; st.push({ m_root.get(), m_root->name });
        while (!st.empty()) {
            const auto cur = st.top(); st.pop();
            for (const auto& inst : cur.node->meshes) fn(inst, cur.node->localTransform, cur.path);
            for (int i = static_cast<int>(cur.node->children.size()) - 1; i >= 0; --i) {
                const auto& ch = cur.node->children[static_cast<size_t>(i)];
                st.push({ ch.get(), cur.path + "/" + ch->name });
            }
        }
    }

    std::shared_ptr<Mesh> Model::FindMeshByInstanceName(const std::string& instanceName) const
    {
        std::shared_ptr<Mesh> found;

        ForEachInstance([&](const MeshInstance& inst, const glm::mat4&, const std::string&) {
            if (!found && inst.name == instanceName)
                found = inst.mesh;
            });

        return found;
    }

    std::shared_ptr<Mesh> Model::FindMeshByPathAndName(const std::string& nodePath, const std::string& instanceName) const
    {
        std::shared_ptr<Mesh> found;
        ForEachInstance([&](const MeshInstance& inst, const glm::mat4&, const std::string& path) {
            if (!found && path == nodePath && inst.name == instanceName)
                found = inst.mesh;
            });
        return found;
    }

    std::shared_ptr<Model> Model::CreateModel(const std::string& path) {
        return std::make_shared<Model>(path);
    }
    std::shared_ptr<Model> Model::CreateModel(std::string name, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
        return std::make_shared<Model>(std::move(name), vertices, indices);
    }
}