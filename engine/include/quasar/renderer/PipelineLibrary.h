#pragma once

/// \file
/// \brief The pipelines that an application shares by name, with the description of their materials.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/UniformBlockLayout.h>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief Everything that a PipelineLibrary needs to add a pipeline and to create its materials.
    struct PipelineDefinition
    {
        ShaderSpecification Shader;              ///< The shader, which the library creates and owns.
        PipelineSpecification State{};           ///< The state of the draws. Its Shader is replaced by the one of the library.
        std::uint32_t UniformBinding = 1;        ///< The binding of the uniform block of the materials.
        UniformBlockLayout Uniforms{};           ///< The members of the uniform block of the materials.
        std::vector<MaterialTexture> Textures{}; ///< The textures of the materials.

        /// Gives its first values to each material that PipelineLibrary::CreateMaterial() creates. Without it, the
        /// values start at zero.
        std::function<void(Material&)> SetDefaults{};
    };

    /// @brief The pipelines that an application shares, each with a name and the description of its materials.
    ///
    /// The library owns its shaders and its pipelines, which stay at the same address until it is destroyed, so it
    /// must outlive the materials that use them. AddDefaults() adds the pipelines of the engine:
    /// - `Unlit`: the color of a texture multiplied by `Color` (vec4), without light.
    /// - `BlinnPhong`: the same color lit by the DirectionalLight of the Renderer, with a highlight of strength
    ///   `Specular` (float) and of sharpness `Shininess` (float).
    ///
    /// Their materials read the texture `Texture` in the slot 0, and start white, with a `Specular` of 0.5 and a
    /// `Shininess` of 32. `Color` is in sRGB, as in a color picker, with a linear alpha. Both write linear colors, see
    /// Renderer.
    class PipelineLibrary
    {
    public:
        static constexpr std::string_view Unlit = "Unlit";           ///< The name of the pipeline without light.
        static constexpr std::string_view BlinnPhong = "BlinnPhong"; ///< The name of the lit pipeline.

        /// @brief Creates an empty library whose pipelines are created with a context, which must outlive it.
        explicit PipelineLibrary(GraphicsContext& context) : m_Context(&context) {}

        ~PipelineLibrary() = default;

        PipelineLibrary(const PipelineLibrary&) = delete;
        PipelineLibrary& operator=(const PipelineLibrary&) = delete;
        PipelineLibrary(PipelineLibrary&&) = delete;
        PipelineLibrary& operator=(PipelineLibrary&&) = delete;

        /// @brief Creates the shader and the pipeline of a definition, under a name.
        /// @return The pipeline, or nullptr if the name is empty or already used, or if the shader or the pipeline cannot
        /// be created. The reason is logged as an engine error.
        Pipeline* Add(std::string name, PipelineDefinition definition);

        /// @brief Adds the pipelines of the engine, from the shaders of the engine resources.
        /// @return True if all of them were added. The reason a pipeline is missing is logged as an engine error.
        bool AddDefaults();

        /// @brief Returns the pipeline of a name, or nullptr if the library has none.
        Pipeline* Get(std::string_view name) const;

        /// @brief Creates a material of a pipeline, with the first values of its definition.
        /// @return The material, or nullptr if the library has no pipeline of this name or if the material cannot be
        /// created. The reason is logged as an engine error.
        [[nodiscard]] Scope<Material> CreateMaterial(std::string_view name) const;

    private:
        struct Entry
        {
            std::string Name;
            PipelineDefinition Definition;
            Scope<Quasar::Shader> Shader;
            Scope<Quasar::Pipeline> Pipeline;
        };

        const Entry* Find(std::string_view name) const;

        GraphicsContext* m_Context;
        std::vector<Entry> m_Entries;
    };
}
