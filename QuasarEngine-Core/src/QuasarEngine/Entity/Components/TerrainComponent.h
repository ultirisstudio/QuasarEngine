#pragma once

#include <memory>
#include <vector>
#include <string>
#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Mesh.h>

namespace QuasarEngine
{
    class TerrainComponent : public Component
    {
    private:
        std::vector<unsigned char> m_HeightPixels;
        int m_ImgW = 0, m_ImgH = 0, m_ImgC = 0;

        std::string m_HeightMapId;
        std::string m_HeightMapPath;

        bool m_Generated = false;
        std::shared_ptr<Mesh> m_Mesh;

        float sampleHeightBilinear(float u, float v) const;

    public:
        TerrainComponent();

        bool  m_PolygonMode = false;
        int   rez = 10;
        int   textureScale = 1;
        float heightMult = 16.0f;

        const std::string& GetHeightMapId()   const { return m_HeightMapId; }
        const std::string& GetHeightMapPath() const { return m_HeightMapPath; }
        bool HasHeightMap() const { return !m_HeightMapPath.empty() && !m_HeightMapId.empty(); }

        void SetHeightMap(const std::string& assetId, const std::string& absPath) {
            m_HeightMapId = assetId;
            m_HeightMapPath = absPath;
        }

        void ClearHeightMap() { m_HeightMapId.clear(); m_HeightMapPath.clear(); }

        bool IsGenerated() const { return m_Generated; }
        std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }

        void GenerateTerrain();
    };
}
