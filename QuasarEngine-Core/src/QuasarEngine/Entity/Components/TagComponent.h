#pragma once

#include <string>
#include <cstdint>
#include <type_traits>

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
    enum class TagMask : uint64_t
    {
        None = 0,
        Player = 1ull << 0,
        Enemy = 1ull << 1,
        NPC = 1ull << 2,
        Collectible = 1ull << 3,
        Trigger = 1ull << 4,
        Static = 1ull << 5,
        Dynamic = 1ull << 6,
        Boss = 1ull << 7,
        Projectile = 1ull << 8,
        
        All = ~0ull
    };

    [[nodiscard]] constexpr TagMask operator|(TagMask a, TagMask b) {
        return static_cast<TagMask>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    [[nodiscard]] constexpr TagMask operator&(TagMask a, TagMask b) {
        return static_cast<TagMask>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
    }

    [[nodiscard]] constexpr TagMask operator~(TagMask a) {
        return static_cast<TagMask>(~static_cast<uint64_t>(a));
    }

    inline TagMask& operator|=(TagMask& a, TagMask b) {
        a = a | b; return a;
    }

    inline TagMask& operator&=(TagMask& a, TagMask b) {
        a = a & b; return a;
    }

    class TagComponent : public Component
    {
    public:
        std::string Tag;
        TagMask     Mask = TagMask::None;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        explicit TagComponent(const std::string& tag) : Tag(tag) {}

        void Add(TagMask flags) { Mask |= flags; }
        void Remove(TagMask flags) { Mask &= ~flags; }
        void Set(TagMask flags) { Mask = flags; }

        
        [[nodiscard]] bool HasAny(TagMask flags) const {
            return (static_cast<uint64_t>(Mask) & static_cast<uint64_t>(flags)) != 0ull;
        }
        
        [[nodiscard]] bool HasAll(TagMask flags) const {
            return (static_cast<uint64_t>(Mask) & static_cast<uint64_t>(flags)) == static_cast<uint64_t>(flags);
        }

        [[nodiscard]] bool IsEnemy() const { return HasAny(TagMask::Enemy); }
        [[nodiscard]] bool IsPlayer() const { return HasAny(TagMask::Player); }
    };
}
