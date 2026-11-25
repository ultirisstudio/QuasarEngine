#include <quasar/localization/TextLayout.h>

#include <doctest/doctest.h>

#include <ostream>
#include <string>
#include <string_view>

using Quasar::TextDirection;

namespace
{
    std::string Utf8(std::u32string_view text)
    {
        std::string result;
        for (const char32_t character : text)
        {
            if (character < 0x80)
            {
                result += static_cast<char>(character);
            }
            else if (character < 0x800)
            {
                result += static_cast<char>(0xC0U | (character >> 6U));
                result += static_cast<char>(0x80U | (character & 0x3FU));
            }
            else
            {
                result += static_cast<char>(0xE0U | (character >> 12U));
                result += static_cast<char>(0x80U | ((character >> 6U) & 0x3FU));
                result += static_cast<char>(0x80U | (character & 0x3FU));
            }
        }
        return result;
    }
}

TEST_SUITE("TextLayout")
{
    TEST_CASE("The languages of the Middle East are written from right to left")
    {
        for (const std::string_view locale : {"ar_SY", "AR", "he", "fa-IR", "ur_PK"})
        {
            CAPTURE(locale);
            CHECK(Quasar::GetTextDirection(locale) == TextDirection::RightToLeft);
        }
        for (const std::string_view locale : {"en_US", "fr_FR", "ja_JP", "", "ara"})
        {
            CAPTURE(locale);
            CHECK(Quasar::GetTextDirection(locale) == TextDirection::LeftToRight);
        }
    }

    TEST_CASE("A text has a part written from right to left if it has a letter of such a language")
    {
        CHECK(Quasar::HasRightToLeftText(Utf8(U"File \u0645\u0644\u0641")));
        CHECK(Quasar::HasRightToLeftText(Utf8(U"\u05D0")));
        CHECK_FALSE(Quasar::HasRightToLeftText(Utf8(U"Fichier \u30D5\u30A1\u30A4\u30EB")));
        CHECK_FALSE(Quasar::HasRightToLeftText(""));
    }

    TEST_CASE("Each Arabic letter takes the form that joins it to its neighbors")
    {
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0645\u0644\u0641")) == Utf8(U"\uFEE3\uFEE0\uFED2"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u062F\u0627\u0631")) == Utf8(U"\uFEA9\uFE8D\uFEAD"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0628")) == Utf8(U"\uFE8F"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0628\u0628 \u0628\u0628")) == Utf8(U"\uFE91\uFE90 \uFE91\uFE90"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u067E\u06AF\u06CC")) == Utf8(U"\uFB58\uFB95\uFBFD"));
    }

    TEST_CASE("Lam and alef become one letter, which joins the letter before it")
    {
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0644\u0627")) == Utf8(U"\uFEFB"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0633\u0644\u0627\u0645")) == Utf8(U"\uFEB3\uFEFC\uFEE1"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0644\u0623 \u0644\u0625 \u0644\u0622")) == Utf8(U"\uFEF7 \uFEF9 \uFEF5"));
    }

    TEST_CASE("The marks above and below the letters do not break their joins")
    {
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0642\u0627\u0644\u0628\u064B\u0627")) == Utf8(U"\uFED7\uFE8E\uFEDF\uFE92\u064B\uFE8E"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0628\u064F")) == Utf8(U"\uFE8F\u064F"));
        CHECK(Quasar::ShapeArabic(Utf8(U"\u0628\u0640\u0628")) == Utf8(U"\uFE91\u0640\uFE90"));
    }

    TEST_CASE("The texts without Arabic letters are not changed by the shaping")
    {
        const std::string text = Utf8(U"File \u30D5\u30A1\u30A4\u30EB \u05D0\u05D1 (1)");
        CHECK(Quasar::ShapeArabic(text) == text);
        CHECK(Quasar::ShapeArabic("") == "");
    }

    TEST_CASE("A text written from right to left is reversed, and its numbers and Latin words keep their order")
    {
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0627\u0628\u062A"), TextDirection::RightToLeft) == Utf8(U"\u062A\u0628\u0627"));
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0645\u0644\u0641 C:/Games"), TextDirection::RightToLeft) == Utf8(U"C:/Games \u0641\u0644\u0645"));
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0639\u062F\u062F 12"), TextDirection::RightToLeft) == Utf8(U"12 \u062F\u062F\u0639"));
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0645\u0634\u0631\u0648\u0639..."), TextDirection::RightToLeft) == Utf8(U"...\u0639\u0648\u0631\u0634\u0645"));
    }

    TEST_CASE("The punctuation between parts of both directions follows the direction of the paragraph")
    {
        CHECK(Quasar::ReorderForDisplay("Hello.", TextDirection::RightToLeft) == ".Hello");
        CHECK(Quasar::ReorderForDisplay("Hello.", TextDirection::LeftToRight) == "Hello.");
        CHECK(Quasar::ReorderForDisplay(Utf8(U"File \u0627\u0628"), TextDirection::LeftToRight) == Utf8(U"File \u0628\u0627"));
    }

    TEST_CASE("The brackets of a text written from right to left are mirrored, and the marks stay after their letters")
    {
        CHECK(Quasar::ReorderForDisplay(Utf8(U"(\u0623)"), TextDirection::RightToLeft) == Utf8(U"(\u0623)"));
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0628\u064B\u0627"), TextDirection::RightToLeft) == Utf8(U"\u0627\u0628\u064B"));
    }

    TEST_CASE("Each line is ordered on its own")
    {
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0627\u0628\n\u062A\u062B"), TextDirection::RightToLeft) == Utf8(U"\u0628\u0627\n\u062B\u062A"));
        CHECK(Quasar::ReorderForDisplay(Utf8(U"\u0627\n\n"), TextDirection::RightToLeft) == Utf8(U"\u0627\n\n"));
        CHECK(Quasar::ReorderForDisplay("", TextDirection::RightToLeft) == "");
    }

    TEST_CASE("A text is shaped and ordered for display, and a text written from left to right is kept as it is")
    {
        CHECK(Quasar::PrepareForDisplay(Utf8(U"\u0645\u0644\u0641"), TextDirection::RightToLeft) == Utf8(U"\uFED2\uFEE0\uFEE3"));
        CHECK(Quasar::PrepareForDisplay(Utf8(U"\u0633\u0644\u0627\u0645 1"), TextDirection::LeftToRight) == Utf8(U"1 \uFEE1\uFEFC\uFEB3"));
        const std::string english = "New Project... (1)";
        CHECK(Quasar::PrepareForDisplay(english, TextDirection::LeftToRight) == english);
    }

    TEST_CASE("A text that is not valid UTF-8 shows a replacement character in place of the bytes that are not valid")
    {
        CHECK(Quasar::ShapeArabic("a\xFF"
                                  "b") == Utf8(U"a\uFFFDb"));
        CHECK(Quasar::ShapeArabic("\xD9") == Utf8(U"\uFFFD"));
        CHECK(Quasar::ShapeArabic("\xF0\x9F\x98\x80") == "\xF0\x9F\x98\x80");
    }
}
