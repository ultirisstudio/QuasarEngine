#pragma once

#include <optional>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/BoneInfo.h>

namespace QuasarEngine
{
    struct ModelImportOptions {
        bool loadPositions = true;
        bool loadNormals = true;
        bool generateNormals = true;
        bool loadTangents = false;
        bool generateTangents = false;
        bool loadTexcoords0 = true;
        bool flipUVs = false;

        bool loadMaterials = true;

        bool loadSkinning = true;
        bool loadAnimations = true;
        int  maxBoneWeights = QE_MAX_BONE_INFLUENCE;

        bool improveCacheLocality = true;
        bool joinIdenticalVertices = true;
        bool triangulate = true;
        bool genUVIfMissing = true;

        std::optional<BufferLayout> vertexLayout;
        DrawMode drawMode = DrawMode::TRIANGLES;

        bool buildMeshes = true;
    };

    struct ModelLoadedInfo {
        struct MeshInfo {
            std::string nodePath;
            std::string name;
            std::shared_ptr<class Mesh> mesh;
            MaterialSpecification material;
            bool skinned = false;
        };

        std::vector<MeshInfo> meshes;
        std::unordered_map<std::string, BoneInfo> bones;
        glm::mat4 globalInverse{ 1.f };
    };
}
