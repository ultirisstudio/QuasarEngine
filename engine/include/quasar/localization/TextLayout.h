#pragma once

/// \file
/// \brief Prepares the texts written from right to left, such as Arabic, for a renderer that draws from left to right,
/// such as ImGui.
/// \author Kurogenshi
/// \date 2026-11-25

#include <cstdint>
#include <string>
#include <string_view>

namespace Quasar
{
    /// @brief The direction in which a language is written.
    enum class TextDirection : std::uint8_t
    {
        LeftToRight, ///< Such as English, French or Japanese.
        RightToLeft  ///< Such as Arabic, Hebrew or Persian.
    };

    /// @brief Returns the direction of the language of a locale, such as RightToLeft for "ar_SY" or "he".
    TextDirection GetTextDirection(std::string_view locale);

    /// @brief Returns true if a text has a letter written from right to left.
    bool HasRightToLeftText(std::string_view text);

    /// @brief Gives each Arabic letter the form that joins it to its neighbors, and joins lam and alef into one letter.
    ///
    /// An Arabic letter changes shape at the start, in the middle and at the end of a word. Fonts draw these shapes from
    /// the presentation forms of Unicode, which this function uses, since ImGui draws each character on its own. The
    /// letters of Persian and Urdu are shaped too. The marks above and below the letters, such as the short vowels, do
    /// not break the joins. The text stays in reading order.
    /// @param text A text in UTF-8.
    /// @return The shaped text in UTF-8.
    std::string ShapeArabic(std::string_view text);

    /// @brief Orders the characters of a text in the order to draw them from left to right, following the Unicode
    /// bidirectional algorithm.
    ///
    /// The parts written from right to left are reversed, and the numbers and the parts written from left to right,
    /// such as paths, keep their order inside them. The brackets of the parts written from right to left are mirrored,
    /// and each mark stays after the letter that it belongs to. Each line is ordered on its own.
    /// @param text A text in UTF-8, in reading order.
    /// @param direction The direction of the paragraph, which places the punctuation between parts of both directions.
    /// @return The text in UTF-8, in drawing order.
    std::string ReorderForDisplay(std::string_view text, TextDirection direction);

    /// @brief Shapes the Arabic letters of a text, then orders it for a renderer that draws from left to right. See
    /// ShapeArabic() and ReorderForDisplay().
    ///
    /// A text written from left to right, without any letter written from right to left, is returned as it is.
    std::string PrepareForDisplay(std::string_view text, TextDirection direction);
}
