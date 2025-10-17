#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    struct SpriteSpecification
    {
        std::optional<std::string> TextureId;
        std::optional<TextureSpecification> TextureSpec;

        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

        glm::vec4 UV{ 0.0f, 0.0f, 1.0f, 1.0f };

        glm::vec2 Tiling{ 1.0f, 1.0f };
        glm::vec2 Offset{ 0.0f, 0.0f };

        bool Visible = true;
        bool FlipX = false;
        bool FlipY = false;

        int SortingOrder = 0;
    };

    class SpriteComponent : public Component
    {
    public:
        SpriteComponent() = default;
        explicit SpriteComponent(const SpriteSpecification& spec);
        ~SpriteComponent() = default;

        SpriteSpecification& GetSpecification()       noexcept { return m_Spec; }
        const SpriteSpecification& GetSpecification() const noexcept { return m_Spec; }

        void SetTexture(Texture2D* texture) noexcept;
        void SetTexture(std::string_view idProject);
        void ResetTexture() noexcept;

        [[nodiscard]] Texture* GetTexture() const noexcept;
        [[nodiscard]] bool HasTexture() const noexcept;

        [[nodiscard]] glm::vec4 GetEffectiveUV() const noexcept;

        std::uint32_t m_Generation = 0;

    private:
        void touch() noexcept { ++m_Generation; }

        SpriteSpecification m_Spec{};
        Texture2D* m_OverrideTexture = nullptr;
    };
}