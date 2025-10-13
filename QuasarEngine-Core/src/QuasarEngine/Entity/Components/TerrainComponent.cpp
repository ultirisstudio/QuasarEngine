#include "qepch.h"
#include "TerrainComponent.h"

#include <stb_image.h>
#include <glm/glm.hpp>

#include <QuasarEngine/Tools/Math.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    TerrainComponent::TerrainComponent() {}

    float TerrainComponent::sampleHeightBilinear(float u, float v) const
    {
        if (m_ImgW <= 0 || m_ImgH <= 0 || m_HeightPixels.empty()) return 0.0f;

        u = glm::clamp(u, 0.0f, 1.0f);
        v = glm::clamp(v, 0.0f, 1.0f);

        float x = u * float(m_ImgW - 1);
        float y = v * float(m_ImgH - 1);

        int x0 = int(floorf(x)), y0 = int(floorf(y));
        int x1 = std::min(x0 + 1, m_ImgW - 1);
        int y1 = std::min(y0 + 1, m_ImgH - 1);

        float tx = x - float(x0);
        float ty = y - float(y0);

        auto readPix = [&](int px, int py) -> float {
            const unsigned char* p = &m_HeightPixels[(py * m_ImgW + px) * m_ImgC];
            if (m_ImgC == 1) return p[0] / 255.0f;
            if (m_ImgC >= 3) return (p[0] + p[1] + p[2]) / (3.0f * 255.0f);
            return 0.0f;
            };

        float h00 = readPix(x0, y0);
        float h10 = readPix(x1, y0);
        float h01 = readPix(x0, y1);
        float h11 = readPix(x1, y1);

        float hx0 = Math::lerp(h00, h10, tx);
        float hx1 = Math::lerp(h01, h11, tx);
        return Math::lerp(hx0, hx1, ty);
    }

    void TerrainComponent::GenerateTerrain()
    {
        m_Generated = false;
        m_Mesh.reset();

        m_HeightPixels.clear();
        m_ImgW = m_ImgH = m_ImgC = 0;

        if (m_HeightMapPath.empty())
        {
            Q_ERROR("TerrainComponent: HeightMap path is empty.");
            return;
        }

        int w = 0, h = 0, c = 0;
        stbi_uc* data = stbi_load(m_HeightMapPath.c_str(), &w, &h, &c, 0);
        if (!data)
        {
            Q_ERROR("TerrainComponent: failed to load heightmap " + m_HeightMapPath);
            return;
        }
        m_ImgW = w; m_ImgH = h; m_ImgC = c;
        m_HeightPixels.assign(data, data + (m_ImgW * m_ImgH * m_ImgC));
        stbi_image_free(data);

        const int vxCountX = rez + 1;
        const int vxCountZ = rez + 1;

        const float sizeX = float(m_ImgW);
        const float sizeZ = float(m_ImgH);
        const float halfX = 0.5f * sizeX;
        const float halfZ = 0.5f * sizeZ;

        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(size_t(vxCountX) * size_t(vxCountZ) * 8);
        indices.reserve(size_t(rez) * size_t(rez) * 4);

        for (int iz = 0; iz < vxCountZ; ++iz)
        {
            for (int ix = 0; ix < vxCountX; ++ix)
            {
                float u = float(ix) / float(rez);
                float v = float(iz) / float(rez);

                float x = -halfX + u * sizeX;
                float z = -halfZ + v * sizeZ;

                float y = 0.0f;
                glm::vec3 n(0.0f, 1.0f, 0.0f);

                //float uu = u * float(textureScale);
                //float vv = v * float(textureScale);

                float uu = u;
                float vv = v;

                vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
                vertices.push_back(uu); vertices.push_back(vv);
            }
        }

        auto idx = [&](int ix, int iz) { return unsigned(iz * vxCountX + ix); };

        for (int iz = 0; iz < rez; ++iz) {
            for (int ix = 0; ix < rez; ++ix) {
                unsigned i0 = idx(ix, iz);
                unsigned i1 = idx(ix + 1, iz);
                unsigned i2 = idx(ix, iz + 1);
                unsigned i3 = idx(ix + 1, iz + 1);

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        m_Mesh = std::make_shared<Mesh>(
            vertices,
            indices,
            std::nullopt,
            DrawMode::PATCHES
        );

        m_Generated = true;
    }
}