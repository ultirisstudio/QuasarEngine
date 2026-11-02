#pragma once

/// \file
/// \brief The look of the ImGui interface: colors, spacing and font.
/// \author Kurogenshi
/// \date 2026-11-02

#include <imgui.h>

#include <filesystem>

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

    /// @brief Returns where the engine font is expected: `resources/fonts/Inter/Inter-Regular.ttf`, in the folder of
    /// the executable. The build copies the engine resources there.
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
}