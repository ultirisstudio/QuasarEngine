#pragma once

/// \file
/// \brief Vertex layouts: how the attributes of a vertex are stored in a vertex buffer.
/// \author Kurogenshi
/// \date 2026-11-18

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace Quasar
{
    /// @brief Type of a vertex attribute, with its GLSL name. The float types are 32-bit floats, and the integer types
    /// 32-bit signed integers.
    enum class VertexAttributeType : std::uint8_t
    {
        Float, ///< `float`
        Vec2,  ///< `vec2`
        Vec3,  ///< `vec3`
        Vec4,  ///< `vec4`
        Int,   ///< `int`
        IVec2, ///< `ivec2`
        IVec3, ///< `ivec3`
        IVec4, ///< `ivec4`
        Mat3,  ///< `mat3`, which takes 3 locations, one per column.
        Mat4   ///< `mat4`, which takes 4 locations, one per column.
    };

    /// @brief Returns the size of an attribute of this type in a vertex, in bytes.
    std::uint32_t GetVertexAttributeSize(VertexAttributeType type);

    /// @brief Returns the number of locations that an attribute of this type takes: the number of columns of a matrix,
    /// and 1 for the other types.
    std::uint32_t GetVertexAttributeLocationCount(VertexAttributeType type);

    /// @brief One attribute of a vertex, for example its position.
    struct VertexAttribute
    {
        /// The `layout(location = N)` of the input of the vertex shader that reads the attribute. A matrix also takes
        /// the next locations, one per column.
        std::uint32_t Location = 0;
        VertexAttributeType Type = VertexAttributeType::Float; ///< The type of the input.
    };

    /// @brief The attributes of a vertex, in the order in which they are stored, without padding between them.
    ///
    /// For example, a vertex `struct { glm::vec3 Position; glm::vec2 TexCoord; }` read by a shader with
    /// `layout(location = 0) in vec3 a_Position;` and `layout(location = 1) in vec2 a_TexCoord;` has the layout
    /// `{{0, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec2}}`.
    class VertexLayout
    {
    public:
        /// The number of locations that every graphics card provides. The locations go from 0 to MaxLocations - 1.
        static constexpr std::uint32_t MaxLocations = 16;

        /// @brief Creates a layout without attributes, which is not valid.
        VertexLayout() = default;

        /// @brief Creates a layout from its attributes, in the order in which they are stored.
        VertexLayout(std::initializer_list<VertexAttribute> attributes);

        /// @brief Returns true if the layout has at least one attribute, and its attributes use locations below
        /// MaxLocations that no other attribute uses. The reason a layout is refused is logged as an engine error.
        bool IsValid() const;

        /// @brief Returns the attributes, in the order in which they are stored.
        const std::vector<VertexAttribute>& GetAttributes() const { return m_Attributes; }

        /// @brief Returns the position of an attribute in a vertex, in bytes.
        /// @param attribute The number of the attribute in GetAttributes(), which must exist.
        std::uint32_t GetOffset(std::size_t attribute) const { return m_Offsets[attribute]; }

        /// @brief Returns the size of a vertex, in bytes.
        std::uint32_t GetStride() const { return m_Stride; }

    private:
        std::vector<VertexAttribute> m_Attributes;
        std::vector<std::uint32_t> m_Offsets;
        std::uint32_t m_Stride = 0;
    };
}
