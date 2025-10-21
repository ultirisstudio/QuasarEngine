#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <array>
#include <memory>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    enum class TextureType : std::uint8_t
    {
        Albedo = 0,
        Normal,
        Metallic,
        Roughness,
        AO,
        Count
    };

    struct MaterialSpecification
    {
        std::optional<std::string> AlbedoTexture;
        std::optional<std::string> NormalTexture;
        std::optional<std::string> MetallicTexture;
        std::optional<std::string> RoughnessTexture;
        std::optional<std::string> AOTexture;

        std::optional<TextureSpecification> AlbedoTextureSpec;
        std::optional<TextureSpecification> NormalTextureSpec;
        std::optional<TextureSpecification> MetallicTextureSpec;
        std::optional<TextureSpecification> RoughnessTextureSpec;
        std::optional<TextureSpecification> AOTextureSpec;

        glm::vec4 Albedo{ 1.0f };
        float Metallic = 0.0f;
        float Roughness = 0.5f;
        float AO = 1.0f;
    };

    class Material
    {
    private:
        static constexpr std::size_t kTexCount = static_cast<std::size_t>(TextureType::Count);
        static constexpr std::size_t idx(TextureType t) noexcept { return static_cast<std::size_t>(t); }

        MaterialSpecification m_Specification;

        std::array<Texture*, kTexCount> m_Overrides{};

        std::optional<std::string>& idRef(TextureType type) noexcept;
        const std::optional<std::string>& idRef(TextureType type) const noexcept;

        std::optional<TextureSpecification>& specRef(TextureType type) noexcept;
        const std::optional<TextureSpecification>& specRef(TextureType type) const noexcept;

        void touch() noexcept { ++m_Generation; }

    public:
        explicit Material(const MaterialSpecification& specification);
        ~Material() = default;

        void SetTexture(TextureType type, Texture* texture) noexcept;
        void SetTexture(TextureType type, std::string_view idProject);

        [[nodiscard]] Texture* GetTexture(TextureType type) const noexcept;
        [[nodiscard]] bool HasTexture(TextureType type) const noexcept;

        glm::vec4& GetAlbedo() noexcept { return m_Specification.Albedo; }
        const glm::vec4& GetAlbedo() const noexcept { return m_Specification.Albedo; }

        float& GetMetallic() noexcept { return m_Specification.Metallic; }
        const float& GetMetallic() const noexcept { return m_Specification.Metallic; }

        float& GetRoughness() noexcept { return m_Specification.Roughness; }
        const float& GetRoughness() const noexcept { return m_Specification.Roughness; }

        float& GetAO() noexcept { return m_Specification.AO; }
        const float& GetAO() const noexcept { return m_Specification.AO; }

        [[nodiscard]] const MaterialSpecification& GetSpecification() const noexcept { return m_Specification; }

        [[nodiscard]] std::optional<std::string> GetTexturePath(TextureType type) const;

        void ResetTexture(TextureType type) noexcept;

        static std::shared_ptr<Material> CreateMaterial(const MaterialSpecification& specification)
        {
            return std::make_shared<Material>(specification);
        }

        std::uint32_t m_ID = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t m_Generation = 0;
    };
}
