#pragma once

/// \file
/// \brief The look of the ImGui interface: colors, spacing and font.
/// \author Kurogenshi
/// \date 2026-11-02

#include <imgui.h>

#include <filesystem>
#include <vector>

/// @brief The look of the ImGui interface. The colors, spacing and font size are defined in ImGuiTheme.cpp, so they
/// can be changed in one place.
namespace Quasar::ImGuiTheme
{
    /// @brief Height of the text in pixels on a standard screen, before the DPI scale.
    inline constexpr float FontSize = 16.0f;

    /// @brief Replaces the colors, spacing and rounding of a style with the engine theme.
    ///
    /// The spacing and the text are multiplied by the DPI scale, so they keep the same physical size on high-DPI
    /// screens.
    /// @param style The style to change, usually ImGui::GetStyle(). Every value is replaced.
    /// @param dpiScale The content scale of the monitor: 1 on a standard screen, 2 on a screen with twice the pixel
    /// density. Values of 0 or less are read as 1.
    void ApplyStyle(ImGuiStyle& style, float dpiScale);

    /// @brief Returns where the engine font is expected: `fonts/Inter/Inter-Regular.ttf`, in the folder of the engine
    /// resources. See GetEngineResourcesDirectory().
    /// @return The path of the font file, or an empty path if the folder of the executable cannot be found.
    std::filesystem::path GetFontPath();

    /// @brief Loads a font file as the default font of an atlas, at FontSize.
    ///
    /// When the file is missing or is not a font, the scalable font embedded in ImGui is used instead, and the reason
    /// is logged as an engine warning.
    /// @param atlas The atlas that receives the font, usually ImGui::GetIO().Fonts. It must not have any font yet.
    /// @param file The TrueType or OpenType file to load.
    /// @return True if the file was loaded, false if the embedded font is used.
    bool LoadFont(ImFontAtlas& atlas, const std::filesystem::path& file);

    /// @brief A font that draws the characters missing from the engine font.
    struct FallbackFont
    {
        std::filesystem::path File; ///< The TrueType or OpenType file.
        float Scale = 1.0f;         ///< The size of its characters compared to the engine font, so that both look the same size.
    };

    /// @brief Returns the fonts that complete the engine font, in the folder of the engine resources: Noto Sans Arabic UI
    /// for Arabic, Persian and Urdu, then Noto Sans JP for Japanese. See GetEngineResourcesDirectory().
    /// @return The fonts, or no font if the folder of the executable cannot be found.
    std::vector<FallbackFont> GetFallbackFonts();

    /// @brief Merges a font into the last font of an atlas, which then draws the characters that it lacks with it.
    ///
    /// ImGui reads the characters of the font when it first draws them, so a large font costs little until it is used.
    /// When the file is missing or is not a font, nothing is merged and the reason is logged as an engine warning.
    /// @param atlas The atlas, usually ImGui::GetIO().Fonts. It must have a font, see LoadFont().
    /// @param font The file and the scale of the font to merge.
    /// @return True if the font was merged.
    bool MergeFont(ImFontAtlas& atlas, const FallbackFont& font);
}
