#include <quasar/renderer/VertexLayout.h>

#include <quasar/log/Log.h>

#include <bitset>
#include <cstdint>
#include <initializer_list>

namespace Quasar
{
    std::uint32_t GetVertexAttributeSize(VertexAttributeType type)
    {
        constexpr std::uint32_t component = 4;
        switch (type)
        {
            case VertexAttributeType::Float:
            case VertexAttributeType::Int:
                return component;
            case VertexAttributeType::Vec2:
            case VertexAttributeType::IVec2:
                return 2 * component;
            case VertexAttributeType::Vec3:
            case VertexAttributeType::IVec3:
                return 3 * component;
            case VertexAttributeType::Vec4:
            case VertexAttributeType::IVec4:
                return 4 * component;
            case VertexAttributeType::Mat3:
                return 9 * component;
            case VertexAttributeType::Mat4:
                return 16 * component;
        }
        return component;
    }

    std::uint32_t GetVertexAttributeLocationCount(VertexAttributeType type)
    {
        switch (type)
        {
            case VertexAttributeType::Mat3:
                return 3;
            case VertexAttributeType::Mat4:
                return 4;
            default:
                return 1;
        }
    }

    VertexLayout::VertexLayout(std::initializer_list<VertexAttribute> attributes) : m_Attributes(attributes)
    {
        m_Offsets.reserve(m_Attributes.size());
        for (const VertexAttribute& attribute : m_Attributes)
        {
            m_Offsets.push_back(m_Stride);
            m_Stride += GetVertexAttributeSize(attribute.Type);
        }
    }

    bool VertexLayout::IsValid() const
    {
        if (m_Attributes.empty())
        {
            QUASAR_ENGINE_ERROR("A vertex layout needs at least one attribute");
            return false;
        }

        std::bitset<MaxLocations> used;
        for (const VertexAttribute& attribute : m_Attributes)
        {
            const std::uint32_t count = GetVertexAttributeLocationCount(attribute.Type);
            if (attribute.Location >= MaxLocations || count > MaxLocations - attribute.Location)
            {
                QUASAR_ENGINE_ERROR("The vertex attribute at location {} does not fit in the locations 0 to {}", attribute.Location, MaxLocations - 1);
                return false;
            }
            for (std::uint32_t location = attribute.Location; location < attribute.Location + count; ++location)
            {
                if (used.test(location))
                {
                    QUASAR_ENGINE_ERROR("The location {} is used by several attributes of a vertex layout", location);
                    return false;
                }
                used.set(location);
            }
        }
        return true;
    }
}
