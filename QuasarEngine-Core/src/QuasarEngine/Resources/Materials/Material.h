// Material.h
#pragma once
#include <string>
#include <optional>
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine {

    enum class TextureType : uint8_t { Albedo, Normal, Metallic, Roughness, AO, Count };

    struct MaterialSpecification {
        // Ids d’assets (projet)
        std::optional<std::string> AlbedoTexture;
        std::optional<std::string> NormalTexture;
        std::optional<std::string> MetallicTexture;
        std::optional<std::string> RoughnessTexture;
        std::optional<std::string> AOTexture;

        // Specs de chargement
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

    using TextureHandle = std::shared_ptr<Texture>; // base

    class Material {
    public:
        explicit Material(const MaterialSpecification& specification);
        ~Material() = default;

        // Setters
        void SetTexture(TextureType type, TextureHandle tex);                 // handle direct
        void SetTexture(TextureType type, const std::string& assetId);        // via AssetManager

        // Getters
        [[nodiscard]] TextureHandle GetTexture(TextureType type) const noexcept;
        [[nodiscard]] bool HasTexture(TextureType type) const noexcept;

        [[nodiscard]] const MaterialSpecification& GetSpecification() const noexcept { return m_Spec; }
        [[nodiscard]] glm::vec4& GetAlbedo() noexcept { return m_Spec.Albedo; }
        [[nodiscard]] float& GetMetallic() noexcept { return m_Spec.Metallic; }
        [[nodiscard]] float& GetRoughness() noexcept { return m_Spec.Roughness; }
        [[nodiscard]] float& GetAO() noexcept { return m_Spec.AO; }

        [[nodiscard]] std::optional<std::string> GetTexturePath(TextureType type) const noexcept;

        void ResetTexture(TextureType type) noexcept;

        static std::shared_ptr<Material> CreateMaterial(const MaterialSpecification& specification);
    
        uint32_t GetObjectId() const noexcept { return m_ID; }
        void SetObjectId(uint32_t id) noexcept { m_ID = id; }

        uint32_t GetGeneration() const noexcept { return m_Generation; }

        void MarkDirty() noexcept { ++m_Generation; }

    private:
        static constexpr size_t Index(TextureType t) noexcept { return static_cast<size_t>(t); }

        // Helpers internes
        std::optional<std::string>& idRef(TextureType);
        const std::optional<std::string>& idRef(TextureType) const;
        std::optional<TextureSpecification>& specRef(TextureType);
        const std::optional<TextureSpecification>& specRef(TextureType) const;

    private:
        MaterialSpecification m_Spec{};
        std::array<TextureHandle, static_cast<size_t>(TextureType::Count)> m_Textures{}; // vide par défaut
        uint32_t m_Generation = 0;
        uint32_t m_ID = 0; // ou std::optional<uint32_t>
    };

} // namespace QuasarEngine
