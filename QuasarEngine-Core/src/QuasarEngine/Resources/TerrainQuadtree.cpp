#include "qepch.h"
#include "TerrainQuadtree.h"

#include <algorithm>
#include <cmath>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    TerrainQuadtree::TerrainQuadtree(
        int   gridW,
        int   gridH,
        float worldSizeX,
        float worldSizeZ,
        float heightScale,
        HeightSampler sampler,
        const TerrainLODSettings& lodSettings)
        : m_gridW(gridW)
        , m_gridH(gridH)
        , m_worldSizeX(worldSizeX)
        , m_worldSizeZ(worldSizeZ)
        , m_heightScale(heightScale)
        , m_sampler(std::move(sampler))
        , m_lod(lodSettings)
    {
        Q_ASSERT(m_gridW > 1 && m_gridH > 1);

        m_cellSizeX = m_worldSizeX / float(m_gridW - 1);
        m_cellSizeZ = m_worldSizeZ / float(m_gridH - 1);
    }

    void TerrainQuadtree::Build()
    {
        const int maxQuadsX = m_gridW - 1;
        const int maxQuadsZ = m_gridH - 1;
        const int rootSize = std::min(maxQuadsX, maxQuadsZ);

        m_root = BuildNode(glm::ivec2(0, 0), rootSize, 0);
    }

    float TerrainQuadtree::SampleHeight01(int gx, int gz) const
    {
        gx = std::clamp(gx, 0, m_gridW - 1);
        gz = std::clamp(gz, 0, m_gridH - 1);
        return std::clamp(m_sampler(gx, gz), 0.0f, 1.0f);
    }

    glm::vec3 TerrainQuadtree::ComputeNormal(int gx, int gz) const
    {
        const int maxX = m_gridW - 1;
        const int maxZ = m_gridH - 1;

        const int gxL = std::clamp(gx - 1, 0, maxX);
        const int gxR = std::clamp(gx + 1, 0, maxX);
        const int gzD = std::clamp(gz - 1, 0, maxZ);
        const int gzU = std::clamp(gz + 1, 0, maxZ);

        const float hL = SampleHeight01(gxL, gz) * m_heightScale;
        const float hR = SampleHeight01(gxR, gz) * m_heightScale;
        const float hD = SampleHeight01(gx, gzD) * m_heightScale;
        const float hU = SampleHeight01(gx, gzU) * m_heightScale;

        glm::vec3 dx(2.0f * m_cellSizeX, hR - hL, 0.0f);
        glm::vec3 dz(0.0f, hU - hD, 2.0f * m_cellSizeZ);

        glm::vec3 n = glm::normalize(glm::cross(dz, dx));
        return n;
    }

    std::unique_ptr<TerrainQuadtree::Node>
        TerrainQuadtree::BuildNode(const glm::ivec2& origin, int size, int depth)
    {
        auto node = std::make_unique<Node>();
        node->origin = origin;
        node->size = size;
        node->depth = depth;

        const bool isLeaf = (depth >= m_lod.maxDepth) || (size <= m_lod.leafNodeSize);

        if (isLeaf)
        {
            glm::vec3 bmin, bmax;
            node->mesh = BuildLeafMesh(origin, size, bmin, bmax);
            node->bboxMin = bmin;
            node->bboxMax = bmax;
            return node;
        }

        const int half = size / 2;
        if (half <= 0)
        {
            glm::vec3 bmin, bmax;
            node->mesh = BuildLeafMesh(origin, size, bmin, bmax);
            node->bboxMin = bmin;
            node->bboxMax = bmax;
            return node;
        }

        glm::vec3 childBMin(std::numeric_limits<float>::max());
        glm::vec3 childBMax(-std::numeric_limits<float>::max());

        glm::ivec2 childOrigins[4] = {
            origin,
            origin + glm::ivec2(half,      0),
            origin + glm::ivec2(0,     half),
            origin + glm::ivec2(half,  half)
        };

        for (int i = 0; i < 4; ++i)
        {
            node->children[i] = BuildNode(childOrigins[i], half, depth + 1);
            if (!node->children[i]) continue;

            childBMin = glm::min(childBMin, node->children[i]->bboxMin);
            childBMax = glm::max(childBMax, node->children[i]->bboxMax);
        }

        node->bboxMin = childBMin;
        node->bboxMax = childBMax;

        return node;
    }

    std::shared_ptr<Mesh> TerrainQuadtree::BuildLeafMesh(
        const glm::ivec2& origin,
        int size,
        glm::vec3& outBMin,
        glm::vec3& outBMax) const
    {
        const int vertCountX = size + 1;
        const int vertCountZ = size + 1;

        std::vector<float>        vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(vertCountX * vertCountZ * 8);

        outBMin = glm::vec3(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
        outBMax = glm::vec3(
            -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max()
        );

        auto idxLocal = [vertCountX](int lx, int lz) -> unsigned int
            {
                return static_cast<unsigned int>(lz * vertCountX + lx);
            };

        for (int lz = 0; lz < vertCountZ; ++lz)
        {
            for (int lx = 0; lx < vertCountX; ++lx)
            {
                const int gx = origin.x + lx;
                const int gz = origin.y + lz;

                const float u = float(gx) / float(m_gridW - 1);
                const float v = float(gz) / float(m_gridH - 1);

                const float h = SampleHeight01(gx, gz) * m_heightScale;

                const float wx = (u - 0.5f) * m_worldSizeX;
                const float wz = (v - 0.5f) * m_worldSizeZ;

                glm::vec3 pos(wx, h, wz);
                glm::vec3 nrm = ComputeNormal(gx, gz);

                outBMin = glm::min(outBMin, pos);
                outBMax = glm::max(outBMax, pos);

                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                vertices.push_back(nrm.x);
                vertices.push_back(nrm.y);
                vertices.push_back(nrm.z);

                vertices.push_back(u);
                vertices.push_back(v);
            }
        }

        for (int lz = 0; lz < size; ++lz)
        {
            for (int lx = 0; lx < size; ++lx)
            {
                const unsigned int i0 = idxLocal(lx, lz);
                const unsigned int i1 = idxLocal(lx + 1, lz);
                const unsigned int i2 = idxLocal(lx, lz + 1);
                const unsigned int i3 = idxLocal(lx + 1, lz + 1);

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        BufferLayout layout = {
            { ShaderDataType::Vec3, "inPosition" },
            { ShaderDataType::Vec3, "inNormal"   },
            { ShaderDataType::Vec2, "inTexCoord" }
        };

        auto mesh = std::make_shared<Mesh>(
            vertices,
            indices,
            layout,
            DrawMode::PATCHES,
            std::nullopt
        );

        return mesh;
    }

    TerrainQuadtree::Frustum
        TerrainQuadtree::ExtractFrustum(const glm::mat4& vp) const
    {
        Frustum f{};
        const glm::mat4 m = vp;

        f.planes[0].x = m[0][3] + m[0][0];
        f.planes[0].y = m[1][3] + m[1][0];
        f.planes[0].z = m[2][3] + m[2][0];
        f.planes[0].w = m[3][3] + m[3][0];

        f.planes[1].x = m[0][3] - m[0][0];
        f.planes[1].y = m[1][3] - m[1][0];
        f.planes[1].z = m[2][3] - m[2][0];
        f.planes[1].w = m[3][3] - m[3][0];

        f.planes[2].x = m[0][3] + m[0][1];
        f.planes[2].y = m[1][3] + m[1][1];
        f.planes[2].z = m[2][3] + m[2][1];
        f.planes[2].w = m[3][3] + m[3][1];

        f.planes[3].x = m[0][3] - m[0][1];
        f.planes[3].y = m[1][3] - m[1][1];
        f.planes[3].z = m[2][3] - m[2][1];
        f.planes[3].w = m[3][3] - m[3][1];

        f.planes[4].x = m[0][3] + m[0][2];
        f.planes[4].y = m[1][3] + m[1][2];
        f.planes[4].z = m[2][3] + m[2][2];
        f.planes[4].w = m[3][3] + m[3][2];

        f.planes[5].x = m[0][3] - m[0][2];
        f.planes[5].y = m[1][3] - m[1][2];
        f.planes[5].z = m[2][3] - m[2][2];
        f.planes[5].w = m[3][3] - m[3][2];

        for (int i = 0; i < 6; ++i)
        {
            glm::vec3 n = glm::vec3(f.planes[i]);
            const float len = glm::length(n);
            if (len > 0.0f)
                f.planes[i] /= len;
        }

        return f;
    }

    bool TerrainQuadtree::BoxInFrustum(
        const Frustum& f,
        const glm::vec3& bmin,
        const glm::vec3& bmax)
    {
        for (int i = 0; i < 6; ++i)
        {
            const glm::vec4& p = f.planes[i];
            const glm::vec3  n(p.x, p.y, p.z);
            const float      d = p.w;

            glm::vec3 positive;
            positive.x = (n.x >= 0.0f) ? bmax.x : bmin.x;
            positive.y = (n.y >= 0.0f) ? bmax.y : bmin.y;
            positive.z = (n.z >= 0.0f) ? bmax.z : bmin.z;

            if (glm::dot(n, positive) + d < 0.0f)
                return false;
        }
        return true;
    }

    bool TerrainQuadtree::ShouldSubdivide(const Node& node,
        const glm::vec3& cameraPos) const
    {
        if (node.depth >= m_lod.maxDepth) return false;
        if (node.size <= m_lod.leafNodeSize) return false;

        const glm::vec3 center = 0.5f * (node.bboxMin + node.bboxMax);
        const float dist = glm::distance(center, cameraPos);

        float limit = m_lod.lodDistances.empty()
            ? std::numeric_limits<float>::max()
            : m_lod.lodDistances[
                std::min<int>(node.depth, int(m_lod.lodDistances.size()) - 1)
            ];

        return dist < limit;
    }

    void TerrainQuadtree::CollectVisible(
        const glm::mat4& viewProj,
        const glm::vec3& cameraPos,
        std::vector<const Node*>& outNodes) const
    {
        outNodes.clear();
        if (!m_root) return;

        const Frustum f = ExtractFrustum(viewProj);
        CollectVisibleRecursive(m_root.get(), f, cameraPos, outNodes);
    }

    void TerrainQuadtree::CollectVisibleRecursive(
        const Node* node,
        const Frustum& frustum,
        const glm::vec3& cameraPos,
        std::vector<const Node*>& outNodes) const
    {
        if (!node) return;

        if (!BoxInFrustum(frustum, node->bboxMin, node->bboxMax))
            return;

        if (node->IsLeaf() || !ShouldSubdivide(*node, cameraPos))
        {
            outNodes.push_back(node);
            return;
        }

        for (int i = 0; i < 4; ++i)
            CollectVisibleRecursive(node->children[i].get(), frustum, cameraPos, outNodes);
    }
}