#pragma once

/// \file
/// \brief Uniform block layouts: where the members of a `layout(std140)` uniform block are stored.
/// \author Kurogenshi
/// \date 2026-11-18

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief Type of a member of a uniform block, with its GLSL name.
    enum class UniformType : std::uint8_t
    {
        Float, ///< `float`
        Int,   ///< `int`, also for a `bool`.
        Vec2,  ///< `vec2`
        Vec3,  ///< `vec3`
        Vec4,  ///< `vec4`
        Mat4   ///< `mat4`
    };

    /// @brief One member of a uniform block.
    struct UniformMember
    {
        std::string Name;                      ///< The name that the values are set with, usually the GLSL name.
        UniformType Type = UniformType::Float; ///< The type of the member.
    };

    /// @brief The members of a `layout(std140)` uniform block, in the order of the GLSL block, with the offsets that
    /// GLSL gives them: a `vec2` starts at a multiple of 8 bytes, and a `vec3`, a `vec4` and a `mat4` at a multiple of
    /// 16 bytes.
    class UniformBlockLayout
    {
    public:
        /// @brief Creates a layout without members.
        UniformBlockLayout() = default;

        /// @brief Creates a layout from its members, in the order of the GLSL block.
        UniformBlockLayout(std::initializer_list<UniformMember> members);

        /// @brief Returns true if every member has a name that no other member has. The reason a layout is refused is
        /// logged as an engine error.
        bool IsValid() const;

        /// @brief Returns the members, in the order of the block.
        const std::vector<UniformMember>& GetMembers() const { return m_Members; }

        /// @brief Returns the number of a member in GetMembers(), or nothing if no member has this name.
        std::optional<std::size_t> Find(std::string_view name) const;

        /// @brief Returns the offset of a member in the block, in bytes.
        /// @param member The number of the member in GetMembers(), which must exist.
        std::uint32_t GetOffset(std::size_t member) const { return m_Offsets[member]; }

        /// @brief Returns the size of the block, in bytes, rounded up to a multiple of 16 as GLSL does.
        std::uint32_t GetSize() const { return m_Size; }

    private:
        std::vector<UniformMember> m_Members;
        std::vector<std::uint32_t> m_Offsets;
        std::uint32_t m_Size = 0;
    };
}
