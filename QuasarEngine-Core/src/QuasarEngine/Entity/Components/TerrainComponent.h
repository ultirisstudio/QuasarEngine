#pragma once

#include <memory>
#include <vector>
#include <string>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Resources/TerrainQuadtree.h>

namespace QuasarEngine
{
    class TerrainComponent : public Component
    {
    private:
        std::vector<unsigned char> m_HeightPixels;
        int m_ImgW = 0, m_ImgH = 0, m_ImgC = 0;

        float m_TerrainSizeX = 1024.0f;
        float m_TerrainSizeZ = 1024.0f;
        float m_HeightScale = 16.0f;

        std::unique_ptr<TerrainQuadtree> m_Quadtree;
        TerrainLODSettings m_LODSettings;
        bool m_UseQuadtree = true;

        std::string m_HeightMapId;
        std::string m_HeightMapPath;

        bool m_Generated = false;
        std::shared_ptr<Mesh> m_Mesh;

        float sampleHeightBilinear(float u, float v) const;
        bool LoadHeightmapIfNeeded();
        void BuildMeshCPUParallel();

    public:
        TerrainComponent();

        bool m_PolygonMode = false;
        int rez = 10;
        int textureScale = 1;
        float heightMult = 16.0f;

        const std::string& GetHeightMapId()   const { return m_HeightMapId; }
        const std::string& GetHeightMapPath() const { return m_HeightMapPath; }
        bool HasHeightMap() const { return !m_HeightMapPath.empty() && !m_HeightMapId.empty(); }

        void SetHeightMap(const std::string& assetId, const std::string& absPath)
        {
            m_HeightMapId = assetId;
            m_HeightMapPath = absPath;
            m_HeightPixels.clear();
            m_ImgW = m_ImgH = m_ImgC = 0;
            m_Generated = false;
            m_Quadtree.reset();
        }

        void ClearHeightMap()
        {
            m_HeightMapId.clear();
            m_HeightMapPath.clear();
            m_HeightPixels.clear();
            m_ImgW = m_ImgH = m_ImgC = 0;
            m_Generated = false;
            m_Quadtree.reset();
        }

        float GetTerrainSizeX() const { return m_TerrainSizeX; }
        float GetTerrainSizeZ() const { return m_TerrainSizeZ; }
        void SetTerrainSize(float x, float z)
        {
            m_TerrainSizeX = std::max(1.0f, x);
            m_TerrainSizeZ = std::max(1.0f, z);
            if (m_Generated && m_UseQuadtree)
                BuildQuadtree();
        }

        float GetHeightScale() const { return m_HeightScale; }
        void SetHeightScale(float h)
        {
            m_HeightScale = std::max(0.0f, h);
            heightMult = m_HeightScale;
            if (m_Generated && m_UseQuadtree)
                BuildQuadtree();
        }

        bool UseQuadtree() const { return m_UseQuadtree; }

        void SetUseQuadtree(bool v)
        {
            if (m_UseQuadtree == v) return;
            m_UseQuadtree = v;

            if (!m_UseQuadtree)
            {
                m_Quadtree.reset();
            }
            else
            {
                if (m_Generated && HasHeightMap())
                    BuildQuadtree();
            }
        }

        bool HasQuadtree() const { return m_Quadtree != nullptr; }
        TerrainQuadtree* GetQuadtree() { return m_Quadtree.get(); }
        const TerrainQuadtree* GetQuadtree() const { return m_Quadtree.get(); }

        TerrainLODSettings& GetLODSettings() { return m_LODSettings; }
        const TerrainLODSettings& GetLODSettings() const { return m_LODSettings; }

        void BuildQuadtree();

        bool IsGenerated() const { return m_Generated; }
        std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }

        void GenerateTerrain();
    };
}
