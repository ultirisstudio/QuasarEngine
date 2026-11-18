#pragma once

/// \file
/// \brief Pipelines: a shader with the depth, culling and blending state that its draws use.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/renderer/Shader.h>

#include <cstdint>

namespace Quasar
{
    /// @brief How the depth of a pixel is compared with the depth already stored, to decide whether it is drawn.
    enum class DepthCompare : std::uint8_t
    {
        Less,           ///< Drawn if it is closer, for most opaque meshes.
        LessOrEqual,    ///< Drawn if it is closer or at the same depth, for example a skybox drawn at the depth 1.
        Greater,        ///< Drawn if it is farther.
        GreaterOrEqual, ///< Drawn if it is farther or at the same depth.
        Equal,          ///< Drawn if it is at the same depth, for a second pass over the same meshes.
        Always          ///< Always drawn.
    };

    /// @brief Which side of the triangles is not drawn. The front of a triangle is the side from which its vertices
    /// turn counter-clockwise.
    enum class CullMode : std::uint8_t
    {
        None,  ///< Both sides are drawn, for example for leaves or sprites.
        Back,  ///< The back is not drawn, for closed meshes whose back faces are hidden.
        Front, ///< The front is not drawn, for example to draw the inside of a mesh.
    };

    /// @brief How the color of a pixel is combined with the color already stored.
    enum class BlendMode : std::uint8_t
    {
        None,    ///< The new color replaces the stored one.
        Alpha,   ///< The new color is laid over the stored one by its alpha, for transparent surfaces.
        Additive ///< The new color, multiplied by its alpha, is added to the stored one, for light effects.
    };

    /// @brief Settings of a pipeline.
    struct PipelineSpecification
    {
        /// The shader of the draws. It must stay alive as long as the pipeline, which does not own it.
        Quasar::Shader* Shader = nullptr;

        bool DepthTest = true;                     ///< Draws only the pixels that pass DepthCompare.
        bool DepthWrite = true;                    ///< Stores the depth of the drawn pixels.
        DepthCompare Compare = DepthCompare::Less; ///< How the depths are compared when DepthTest is on.
        CullMode Culling = CullMode::Back;         ///< Which side of the triangles is not drawn.
        BlendMode Blending = BlendMode::None;      ///< How the colors are combined.
    };

    /// @brief A shader and the state of its draws, as one object that the graphics API can prepare in advance.
    ///
    /// A pipeline is created with GraphicsContext::CreatePipeline(), and Bind() makes the next draws use its shader and
    /// its state. Depths go from 0, the closest, to 1, the farthest, with every graphics API.
    class Pipeline
    {
    public:
        virtual ~Pipeline() = default;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline(Pipeline&&) = delete;
        Pipeline& operator=(Pipeline&&) = delete;

        /// @brief Returns true if a pipeline can be created from the specification, whatever the graphics API. It needs a
        /// shader. The reason a specification is refused is logged as an engine error.
        static bool IsValid(const PipelineSpecification& specification);

        /// @brief Makes the next draws use the shader and the state of the pipeline.
        virtual void Bind() = 0;

        /// @brief Returns the specification of the pipeline.
        const PipelineSpecification& GetSpecification() const { return m_Specification; }

    protected:
        /// @brief Keeps the specification, which must be valid. See IsValid().
        explicit Pipeline(const PipelineSpecification& specification);

    private:
        PipelineSpecification m_Specification;
    };
}
