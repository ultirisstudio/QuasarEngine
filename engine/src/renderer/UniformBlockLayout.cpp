#include <quasar/renderer/UniformBlockLayout.h>

#include <quasar/log/Log.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace Quasar
{
    namespace
    {
        struct TypeLayout
        {
            std::uint32_t Size = 4;
            std::uint32_t Alignment = 4;
        };

        TypeLayout GetTypeLayout(UniformType type)
        {
            switch (type)
            {
                case UniformType::Float:
                case UniformType::Int:
                    return {.Size = 4, .Alignment = 4};
                case UniformType::Vec2:
                    return {.Size = 8, .Alignment = 8};
                case UniformType::Vec3:
                    return {.Size = 12, .Alignment = 16};
                case UniformType::Vec4:
                    return {.Size = 16, .Alignment = 16};
                case UniformType::Mat4:
                    return {.Size = 64, .Alignment = 16};
            }
            return {};
        }

        std::uint32_t AlignUp(std::uint32_t value, std::uint32_t alignment)
        {
            return (value + alignment - 1) / alignment * alignment;
        }
    }

    UniformBlockLayout::UniformBlockLayout(std::initializer_list<UniformMember> members) : m_Members(members)
    {
        m_Offsets.reserve(m_Members.size());
        for (const UniformMember& member : m_Members)
        {
            const TypeLayout layout = GetTypeLayout(member.Type);
            const std::uint32_t offset = AlignUp(m_Size, layout.Alignment);
            m_Offsets.push_back(offset);
            m_Size = offset + layout.Size;
        }
        m_Size = AlignUp(m_Size, 16);
    }

    bool UniformBlockLayout::IsValid() const
    {
        for (std::size_t index = 0; index < m_Members.size(); ++index)
        {
            if (m_Members[index].Name.empty())
            {
                QUASAR_ENGINE_ERROR("A member of a uniform block has no name");
                return false;
            }
            if (Find(m_Members[index].Name) != index)
            {
                QUASAR_ENGINE_ERROR("Several members of a uniform block are named '{}'", m_Members[index].Name);
                return false;
            }
        }
        return true;
    }

    std::optional<std::size_t> UniformBlockLayout::Find(std::string_view name) const
    {
        for (std::size_t index = 0; index < m_Members.size(); ++index)
        {
            if (m_Members[index].Name == name)
            {
                return index;
            }
        }
        return std::nullopt;
    }
}
