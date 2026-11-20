#include <quasar/renderer/Material.h>

#include <quasar/log/Log.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Quasar
{
    namespace
    {
        template <typename T>
        std::span<const std::byte> AsBytes(const T& value)
        {
            return std::as_bytes(std::span<const T, 1>(&value, 1));
        }

        bool HasValidTextures(const MaterialSpecification& specification)
        {
            const std::vector<MaterialTexture>& textures = specification.Textures;
            for (std::size_t index = 0; index < textures.size(); ++index)
            {
                if (textures[index].Name.empty())
                {
                    QUASAR_ENGINE_ERROR("A texture of a material has no name");
                    return false;
                }
                for (std::size_t other = 0; other < index; ++other)
                {
                    if (textures[other].Name == textures[index].Name)
                    {
                        QUASAR_ENGINE_ERROR("Several textures of a material are named '{}'", textures[index].Name);
                        return false;
                    }
                    if (textures[other].Slot == textures[index].Slot)
                    {
                        QUASAR_ENGINE_ERROR("The textures '{}' and '{}' of a material use the same slot {}", textures[other].Name, textures[index].Name,
                                            textures[index].Slot);
                        return false;
                    }
                }
            }
            return true;
        }
    }

    Material::Material(const MaterialSpecification& specification, Scope<UniformBuffer> uniforms, Texture2D* white)
        : m_Specification(specification), m_UniformBuffer(std::move(uniforms)), m_Values(specification.Uniforms.GetSize()), m_White(white),
          m_Textures(specification.Textures.size(), nullptr)
    {
    }

    Scope<Material> Material::Create(GraphicsContext& context, const MaterialSpecification& specification)
    {
        if (specification.Pipeline == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a material without a pipeline");
            return nullptr;
        }
        if (!specification.Uniforms.IsValid() || !HasValidTextures(specification))
        {
            return nullptr;
        }
        if (!specification.Uniforms.GetMembers().empty() && ShaderBindings::IsFilledByRenderer(specification.UniformBinding))
        {
            QUASAR_ENGINE_ERROR("Cannot create a material on the binding {}, which the Renderer fills", specification.UniformBinding);
            return nullptr;
        }
        if (std::ranges::any_of(specification.Textures, [](const MaterialTexture& texture) { return texture.Slot == ShaderBindings::ShadowMap; }))
        {
            QUASAR_ENGINE_ERROR("Cannot create a material that reads a texture from the slot {}, which the Renderer binds", ShaderBindings::ShadowMap);
            return nullptr;
        }

        Scope<UniformBuffer> uniforms;
        if (!specification.Uniforms.GetMembers().empty())
        {
            uniforms = context.CreateUniformBuffer(specification.Uniforms.GetSize(), specification.UniformBinding);
            if (uniforms == nullptr)
            {
                return nullptr;
            }
        }

        Texture2D* white = nullptr;
        if (!specification.Textures.empty())
        {
            white = context.GetWhiteTexture();
            if (white == nullptr)
            {
                return nullptr;
            }
        }
        return Scope<Material>(new Material(specification, std::move(uniforms), white));
    }

    void Material::SetFloat(std::string_view name, float value)
    {
        SetValue(name, UniformType::Float, AsBytes(value));
    }

    void Material::SetInt(std::string_view name, int value)
    {
        SetValue(name, UniformType::Int, AsBytes(value));
    }

    void Material::SetVec2(std::string_view name, glm::vec2 value)
    {
        SetValue(name, UniformType::Vec2, AsBytes(value));
    }

    void Material::SetVec3(std::string_view name, glm::vec3 value)
    {
        SetValue(name, UniformType::Vec3, AsBytes(value));
    }

    void Material::SetVec4(std::string_view name, glm::vec4 value)
    {
        SetValue(name, UniformType::Vec4, AsBytes(value));
    }

    void Material::SetMat4(std::string_view name, const glm::mat4& value)
    {
        SetValue(name, UniformType::Mat4, AsBytes(value));
    }

    void Material::SetTexture(std::string_view name, Texture2D* texture)
    {
        const std::vector<MaterialTexture>& textures = m_Specification.Textures;
        const auto found = std::ranges::find(textures, name, &MaterialTexture::Name);
        if (found == textures.end())
        {
            Report(name, "has no texture named");
            return;
        }
        m_Textures[static_cast<std::size_t>(found - textures.begin())] = texture;
        m_Changed = true;
    }

    void Material::Bind()
    {
        m_Specification.Pipeline->Bind();
        BindResources();
    }

    void Material::BindResources()
    {
        if (m_UniformBuffer != nullptr)
        {
            if (m_ValuesChanged)
            {
                m_UniformBuffer->SetData(m_Values);
                m_ValuesChanged = false;
            }
            m_UniformBuffer->Bind();
        }
        for (std::size_t index = 0; index < m_Textures.size(); ++index)
        {
            Texture2D& texture = m_Textures[index] != nullptr ? *m_Textures[index] : *m_White;
            texture.Bind(m_Specification.Textures[index].Slot);
        }
        m_Changed = false;
    }

    void Material::SetValue(std::string_view name, UniformType type, std::span<const std::byte> value)
    {
        const UniformBlockLayout& layout = m_Specification.Uniforms;
        const std::optional<std::size_t> member = layout.Find(name);
        if (!member)
        {
            Report(name, "has no value named");
            return;
        }
        if (layout.GetMembers()[*member].Type != type)
        {
            Report(name, "has another type for the value");
            return;
        }
        std::ranges::copy(value, std::next(m_Values.begin(), static_cast<std::ptrdiff_t>(layout.GetOffset(*member))));
        m_ValuesChanged = true;
        m_Changed = true;
    }

    void Material::Report(std::string_view name, std::string_view problem)
    {
        if (std::ranges::find(m_ReportedNames, name) != m_ReportedNames.end())
        {
            return;
        }
        m_ReportedNames.emplace_back(name);
        QUASAR_ENGINE_WARN("The material {} '{}'", problem, name);
    }
}
