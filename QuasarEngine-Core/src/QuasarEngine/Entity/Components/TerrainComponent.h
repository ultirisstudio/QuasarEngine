#pragma once

#include <memory>
#include <vector>
#include <string>
#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Resources/Materials/Material.h>

namespace QuasarEngine
{
    class TerrainComponent : public Component
    {
    private:
        std::shared_ptr<Texture2D> m_HeightMapTexture;
        std::string m_HeightMapPath;
        bool m_Generated = false;

        std::shared_ptr<Mesh> m_Mesh;

        std::vector<unsigned char> m_HeightPixels;
        int m_ImgW = 0, m_ImgH = 0, m_ImgC = 0;

        float sampleHeightBilinear(float u, float v) const;

    public:
        TerrainComponent();

        bool  m_PolygonMode = false;
        int   rez = 10;
        int   textureScale = 1;
        float heightMult = 16.0f;

        void SetHeightMap(const std::string& path) { m_HeightMapPath = path; }
        std::string GetHeightMapPath() const { return m_HeightMapPath; }

        bool IsGenerated() const { return m_Generated; }

        std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }

        void GenerateTerrain();

        void Draw() {}
    };
}