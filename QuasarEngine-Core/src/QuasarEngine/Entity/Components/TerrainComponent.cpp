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

        float x = u * (m_ImgW - 1);
        float y = v * (m_ImgH - 1);

        int x0 = static_cast<int>(floorf(x));
        int y0 = static_cast<int>(floorf(y));
        int x1 = glm::min(x0 + 1, m_ImgW - 1);
        int y1 = glm::min(y0 + 1, m_ImgH - 1);

        float tx = x - x0;
        float ty = y - y0;

        auto readPix = [&](int px, int py) -> float {
            const unsigned char* p = &m_HeightPixels[(py * m_ImgW + px) * m_ImgC];
            float val = 0.0f;
            if (m_ImgC == 1) val = p[0] / 255.0f;
            else if (m_ImgC >= 3) val = (p[0] + p[1] + p[2]) / (3.0f * 255.0f);
            return val;
            };

        float h00 = readPix(x0, y0);
        float h10 = readPix(x1, y0);
        float h01 = readPix(x0, y1);
        float h11 = readPix(x1, y1);

        float hx0 = Math::lerp(h00, h10, tx);
        float hx1 = Math::lerp(h01, h11, tx);
        float h = Math::lerp(hx0, hx1, ty);

        return h;
    }

    void TerrainComponent::GenerateTerrain()
    {
        m_Generated = false;
        m_Mesh.reset();
        m_MatSpec.reset();
        m_HeightPixels.clear();
        m_ImgW = m_ImgH = m_ImgC = 0;

        if (m_HeightMapPath.empty())
        {
            Q_ERROR("TerrainComponent: HeightMap path is empty.");
            return;
        }

        stbi_uc* data = stbi_load(m_HeightMapPath.c_str(), &m_ImgW, &m_ImgH, &m_ImgC, 0);
        if (!data)
        {
            Q_ERROR("TerrainComponent: failed to load heightmap " + m_HeightMapPath);
            return;
        }
        m_HeightPixels.assign(data, data + (m_ImgW * m_ImgH * m_ImgC));
        stbi_image_free(data);

        TextureSpecification spec;
        spec.min_filter_param = TextureFilter::LINEAR_MIPMAP_LINEAR;
        spec.mag_filter_param = TextureFilter::LINEAR;
        m_HeightMapTexture = Texture2D::CreateTexture2D(spec);
        m_HeightMapTexture->LoadFromPath(m_HeightMapPath);

        const int vxCountX = rez + 1;
        const int vxCountZ = rez + 1;

        const float sizeX = static_cast<float>(m_ImgW);
        const float sizeZ = static_cast<float>(m_ImgH);

        const float halfX = 0.5f * sizeX;
        const float halfZ = 0.5f * sizeZ;

        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(vxCountX * vxCountZ * 8);
        indices.reserve(rez * rez * 6);

        auto heightAt = [&](int ix, int iz) -> float {
            float u = static_cast<float>(ix) / static_cast<float>(rez);
            float v = static_cast<float>(iz) / static_cast<float>(rez);
            return sampleHeightBilinear(u, v) * heightMult;
            };

        for (int iz = 0; iz < vxCountZ; ++iz)
        {
            for (int ix = 0; ix < vxCountX; ++ix)
            {
                float u = static_cast<float>(ix) / static_cast<float>(rez);
                float v = static_cast<float>(iz) / static_cast<float>(rez);

                float x = -halfX + u * sizeX;
                float z = -halfZ + v * sizeZ;
                float y = heightAt(ix, iz);

                float hxL = heightAt(std::max(ix - 1, 0), iz);
                float hxR = heightAt(std::min(ix + 1, rez), iz);
                float hzD = heightAt(ix, std::max(iz - 1, 0));
                float hzU = heightAt(ix, std::min(iz + 1, rez));

                glm::vec3 dx(2.0f, hxR - hxL, 0.0f);
                glm::vec3 dz(0.0f, hzU - hzD, 2.0f);
                glm::vec3 n = glm::normalize(glm::cross(dz, dx));

                float uu = u * static_cast<float>(textureScale);
                float vv = v * static_cast<float>(textureScale);

                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                vertices.push_back(n.x);
                vertices.push_back(n.y);
                vertices.push_back(n.z);

                vertices.push_back(uu);
                vertices.push_back(vv);
            }
        }

        auto idx = [&](int ix, int iz) { return static_cast<unsigned int>(iz * vxCountX + ix); };

        for (int iz = 0; iz < rez; ++iz)
        {
            for (int ix = 0; ix < rez; ++ix)
            {
                unsigned int i0 = idx(ix, iz);
                unsigned int i1 = idx(ix + 1, iz);
                unsigned int i2 = idx(ix, iz + 1);
                unsigned int i3 = idx(ix + 1, iz + 1);

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);
                
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        MaterialSpecification matSpec{};
        matSpec.Albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        matSpec.Roughness = 0.9f;
        matSpec.Metallic = 0.0f;
        matSpec.AO = 1.0f;

        m_Mesh = std::make_shared<Mesh>(
            vertices,
            indices,
            std::nullopt,
            DrawMode::TRIANGLES,
            matSpec
        );

        m_MatSpec = matSpec;
        m_Generated = true;
        //Q_INFO("Terrain generated: " + std::to_string(m_ImgW) + "x" + std::to_string(m_ImgH) + ", rez=" + std::to_string(rez) + " -> " + std::to_string(vertices.size() / 8) + " vertices, " + std::to_string(indices.size()) + " indices");
    }
}