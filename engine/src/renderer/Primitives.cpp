#include <quasar/renderer/Primitives.h>

#include <quasar/log/Log.h>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cmath>
#include <cstdint>

namespace Quasar::Primitives
{
    namespace
    {
        struct Face
        {
            glm::vec3 Right{0.0f};
            glm::vec3 Up{0.0f};
        };

        bool IsPositive(float value, const char* shape)
        {
            if (!(value > 0.0f) || !std::isfinite(value))
            {
                QUASAR_ENGINE_ERROR("Cannot build a {} of size {}: it must be a finite number above 0", shape, value);
                return false;
            }
            return true;
        }

        void AddFace(MeshData& data, const Face& face, float size)
        {
            const glm::vec3 normal = glm::cross(face.Right, face.Up);
            const glm::vec3 center = normal * (size * 0.5f);
            const glm::vec3 right = face.Right * (size * 0.5f);
            const glm::vec3 up = face.Up * (size * 0.5f);
            const auto first = static_cast<std::uint32_t>(data.Vertices.size());

            data.Vertices.push_back({.Position = center - right + up, .Normal = normal, .TexCoord = {0.0f, 0.0f}});
            data.Vertices.push_back({.Position = center - right - up, .Normal = normal, .TexCoord = {0.0f, 1.0f}});
            data.Vertices.push_back({.Position = center + right - up, .Normal = normal, .TexCoord = {1.0f, 1.0f}});
            data.Vertices.push_back({.Position = center + right + up, .Normal = normal, .TexCoord = {1.0f, 0.0f}});
            data.Indices.insert(data.Indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
        }
    }

    MeshData CreateCube(float size)
    {
        if (!IsPositive(size, "cube"))
        {
            return {};
        }

        constexpr glm::vec3 x(1.0f, 0.0f, 0.0f);
        constexpr glm::vec3 y(0.0f, 1.0f, 0.0f);
        constexpr glm::vec3 z(0.0f, 0.0f, 1.0f);
        constexpr std::array<Face, 6> faces{{
            {.Right = x, .Up = y},
            {.Right = -x, .Up = y},
            {.Right = -z, .Up = y},
            {.Right = z, .Up = y},
            {.Right = x, .Up = -z},
            {.Right = x, .Up = z},
        }};

        MeshData data;
        for (const Face& face : faces)
        {
            AddFace(data, face, size);
        }
        data.GenerateTangents();
        return data;
    }

    MeshData CreateQuad(float size)
    {
        if (!IsPositive(size, "quad"))
        {
            return {};
        }

        MeshData data;
        AddFace(data, {.Right = {1.0f, 0.0f, 0.0f}, .Up = {0.0f, 1.0f, 0.0f}}, size);
        for (Vertex& vertex : data.Vertices)
        {
            vertex.Position.z = 0.0f;
        }
        data.GenerateTangents();
        return data;
    }

    MeshData CreateSphere(float radius, std::uint32_t segments, std::uint32_t rings)
    {
        if (!IsPositive(radius, "sphere"))
        {
            return {};
        }
        if (segments < 3 || rings < 2)
        {
            QUASAR_ENGINE_ERROR("Cannot build a sphere of {} segments and {} rings: it needs at least 3 segments and 2 rings", segments, rings);
            return {};
        }

        MeshData data;
        data.Vertices.reserve(std::size_t{rings + 1} * (segments + 1));
        for (std::uint32_t ring = 0; ring <= rings; ++ring)
        {
            const float v = static_cast<float>(ring) / static_cast<float>(rings);
            const float polar = v * glm::pi<float>();
            const bool pole = ring == 0 || ring == rings;
            for (std::uint32_t segment = 0; segment <= segments; ++segment)
            {
                const float middle = pole && segment < segments ? 0.5f : 0.0f;
                const float u = (static_cast<float>(segment) + middle) / static_cast<float>(segments);
                const float azimuth = u * glm::two_pi<float>();
                const glm::vec3 normal(std::sin(polar) * std::sin(azimuth), std::cos(polar), std::sin(polar) * std::cos(azimuth));
                data.Vertices.push_back({.Position = normal * radius, .Normal = normal, .TexCoord = {u, v}});
            }
        }

        const std::uint32_t columns = segments + 1;
        for (std::uint32_t ring = 0; ring < rings; ++ring)
        {
            for (std::uint32_t segment = 0; segment < segments; ++segment)
            {
                const std::uint32_t topLeft = ring * columns + segment;
                const std::uint32_t bottomLeft = topLeft + columns;
                if (ring == 0)
                {
                    data.Indices.insert(data.Indices.end(), {topLeft, bottomLeft, bottomLeft + 1});
                }
                else if (ring == rings - 1)
                {
                    data.Indices.insert(data.Indices.end(), {topLeft, bottomLeft, topLeft + 1});
                }
                else
                {
                    data.Indices.insert(data.Indices.end(), {topLeft, bottomLeft, topLeft + 1, topLeft + 1, bottomLeft, bottomLeft + 1});
                }
            }
        }
        data.GenerateTangents();
        return data;
    }
}
