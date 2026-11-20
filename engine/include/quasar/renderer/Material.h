#pragma once

/// \file
/// \brief Materials: a pipeline with the values and the textures that its shader reads.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/ShaderBindings.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/UniformBlockLayout.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief One texture that a material gives to its shader.
    struct MaterialTexture
    {
        std::string Name;       ///< The name that the texture is set with.
        std::uint32_t Slot = 0; ///< The N of the `layout(binding = N) uniform sampler2D` of the shader.
    };

    /// @brief The description of a material: its pipeline, the members of its uniform block and its textures.
    ///
    /// Several materials can share a description, for example a red and a blue material of the same shader.
    struct MaterialSpecification
    {
        /// The pipeline of the draws. It must stay alive as long as the material, which does not own it.
        Quasar::Pipeline* Pipeline = nullptr;

        /// The N of the `layout(std140, binding = N) uniform` block that holds the values, which cannot be one that the
        /// Renderer fills. See ShaderBindings.
        std::uint32_t UniformBinding = ShaderBindings::Material;

        UniformBlockLayout Uniforms{};           ///< The members of the block, in the order of the GLSL block.
        std::vector<MaterialTexture> Textures{}; ///< The textures of the shader.
    };

    /// @brief The values and the textures of a material, with the pipeline that draws with them.
    ///
    /// The values start at zero, and a texture that is not set is the white texture of the context, see
    /// GraphicsContext::GetWhiteTexture(). The Set functions only change the copy of the
    /// material, and Bind() sends the values that changed. A name that the description does not have, or a value of
    /// another type than the member, is refused with one engine warning per name.
    class Material
    {
    public:
        /// @brief Creates a material from its description.
        /// @return The material, or nullptr if the description has no pipeline, has two members or two textures with
        /// the same name, has two textures in the same slot, uses a block or a texture slot that the Renderer fills, or
        /// if the graphics API cannot create its buffer. The reason is logged as an engine error.
        static Scope<Material> Create(GraphicsContext& context, const MaterialSpecification& specification);

        ~Material() = default;

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;
        Material(Material&&) = delete;
        Material& operator=(Material&&) = delete;

        void SetFloat(std::string_view name, float value);           ///< Sets a member of type UniformType::Float.
        void SetInt(std::string_view name, int value);               ///< Sets a member of type UniformType::Int.
        void SetVec2(std::string_view name, glm::vec2 value);        ///< Sets a member of type UniformType::Vec2.
        void SetVec3(std::string_view name, glm::vec3 value);        ///< Sets a member of type UniformType::Vec3.
        void SetVec4(std::string_view name, glm::vec4 value);        ///< Sets a member of type UniformType::Vec4.
        void SetMat4(std::string_view name, const glm::mat4& value); ///< Sets a member of type UniformType::Mat4.

        /// @brief Sets a texture, or white with nullptr. The material does not own it, so it must stay alive while the
        /// material uses it.
        void SetTexture(std::string_view name, Texture2D* texture);

        /// @brief Makes the next draws use the pipeline, the values and the textures of the material.
        void Bind();

        /// @brief Makes the next draws use the values and the textures of the material, without binding its pipeline,
        /// for example when the pipeline is already bound.
        void BindResources();

        /// @brief Returns whether a value or a texture was set since the last Bind() or BindResources(). A new material
        /// counts as changed.
        bool HasChanged() const { return m_Changed; }

        /// @brief Returns the description of the material.
        const MaterialSpecification& GetSpecification() const { return m_Specification; }

    private:
        Material(const MaterialSpecification& specification, Scope<UniformBuffer> uniforms, Texture2D* white);

        void SetValue(std::string_view name, UniformType type, std::span<const std::byte> value);
        void Report(std::string_view name, std::string_view problem);

        MaterialSpecification m_Specification;
        Scope<UniformBuffer> m_UniformBuffer;
        std::vector<std::byte> m_Values;
        bool m_ValuesChanged = false;
        bool m_Changed = true;
        Texture2D* m_White = nullptr;
        std::vector<Texture2D*> m_Textures;
        std::vector<std::string> m_ReportedNames;
    };
}
