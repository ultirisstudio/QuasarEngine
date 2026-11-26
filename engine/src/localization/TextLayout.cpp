#include <quasar/localization/TextLayout.h>

#include <SheenBidi/SheenBidi.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>

namespace Quasar
{
    namespace
    {
        constexpr char32_t Replacement = 0xFFFD;
        constexpr char32_t Lam = 0x0644;
        constexpr char32_t ZeroWidthJoiner = 0x200D;
        constexpr char32_t Tatweel = 0x0640;

        constexpr std::array<std::string_view, 12> RightToLeftLanguages{"ar", "he", "iw", "fa", "ur", "yi", "ps", "sd", "ug", "dv", "ckb", "syr"};

        struct ArabicLetter
        {
            char32_t Letter;
            char32_t Isolated;
            char32_t Final;
            char32_t Initial;
            char32_t Medial;
        };

        constexpr std::array<ArabicLetter, 58> ArabicLetters{{
            {0x0621, 0xFE80, 0, 0, 0},
            {0x0622, 0xFE81, 0xFE82, 0, 0},
            {0x0623, 0xFE83, 0xFE84, 0, 0},
            {0x0624, 0xFE85, 0xFE86, 0, 0},
            {0x0625, 0xFE87, 0xFE88, 0, 0},
            {0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C},
            {0x0627, 0xFE8D, 0xFE8E, 0, 0},
            {0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92},
            {0x0629, 0xFE93, 0xFE94, 0, 0},
            {0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98},
            {0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C},
            {0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0},
            {0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4},
            {0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8},
            {0x062F, 0xFEA9, 0xFEAA, 0, 0},
            {0x0630, 0xFEAB, 0xFEAC, 0, 0},
            {0x0631, 0xFEAD, 0xFEAE, 0, 0},
            {0x0632, 0xFEAF, 0xFEB0, 0, 0},
            {0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4},
            {0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8},
            {0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC},
            {0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0},
            {0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4},
            {0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8},
            {0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC},
            {0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0},
            {0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4},
            {0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8},
            {0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC},
            {0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0},
            {0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4},
            {0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8},
            {0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC},
            {0x0648, 0xFEED, 0xFEEE, 0, 0},
            {0x0649, 0xFEEF, 0xFEF0, 0, 0},
            {0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4},
            {0x0671, 0xFB50, 0xFB51, 0, 0},
            {0x0679, 0xFB66, 0xFB67, 0xFB68, 0xFB69},
            {0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59},
            {0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D},
            {0x0688, 0xFB88, 0xFB89, 0, 0},
            {0x0691, 0xFB8C, 0xFB8D, 0, 0},
            {0x0698, 0xFB8A, 0xFB8B, 0, 0},
            {0x06A4, 0xFB6A, 0xFB6B, 0xFB6C, 0xFB6D},
            {0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91},
            {0x06AD, 0xFBD3, 0xFBD4, 0xFBD5, 0xFBD6},
            {0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95},
            {0x06BA, 0xFB9E, 0xFB9F, 0, 0},
            {0x06BE, 0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD},
            {0x06C1, 0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9},
            {0x06C6, 0xFBD9, 0xFBDA, 0, 0},
            {0x06C7, 0xFBD7, 0xFBD8, 0, 0},
            {0x06C8, 0xFBDB, 0xFBDC, 0, 0},
            {0x06CB, 0xFBDE, 0xFBDF, 0, 0},
            {0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF},
            {0x06D0, 0xFBE4, 0xFBE5, 0xFBE6, 0xFBE7},
            {0x06D2, 0xFBAE, 0xFBAF, 0, 0},
            {0x06D3, 0xFBB0, 0xFBB1, 0, 0},
        }};

        constexpr std::array<std::array<char32_t, 3>, 4> LamAlefs{
            {{0x0622, 0xFEF5, 0xFEF6}, {0x0623, 0xFEF7, 0xFEF8}, {0x0625, 0xFEF9, 0xFEFA}, {0x0627, 0xFEFB, 0xFEFC}}};

        static_assert(std::ranges::is_sorted(ArabicLetters, {}, &ArabicLetter::Letter), "FindLetter() searches the letters by halves");

        enum class Joining : std::uint8_t
        {
            None,
            Right,
            Dual,
            Causing,
            Transparent
        };

        const ArabicLetter* FindLetter(char32_t character)
        {
            const auto index = static_cast<std::size_t>(
                std::ranges::distance(ArabicLetters.begin(), std::ranges::lower_bound(ArabicLetters, character, {}, &ArabicLetter::Letter)));
            return index < ArabicLetters.size() && ArabicLetters.at(index).Letter == character ? &ArabicLetters.at(index) : nullptr;
        }

        bool IsInRange(char32_t character, char32_t first, char32_t last)
        {
            return character >= first && character <= last;
        }

        bool IsArabicMark(char32_t character)
        {
            return IsInRange(character, 0x0610, 0x061A) || IsInRange(character, 0x064B, 0x065F) || character == 0x0670 ||
                   IsInRange(character, 0x06D6, 0x06DC) || IsInRange(character, 0x06DF, 0x06E4) || IsInRange(character, 0x06E7, 0x06E8) ||
                   IsInRange(character, 0x06EA, 0x06ED) || IsInRange(character, 0x08D3, 0x08FF);
        }

        Joining GetJoining(char32_t character)
        {
            if (IsArabicMark(character))
            {
                return Joining::Transparent;
            }
            if (character == Tatweel || character == ZeroWidthJoiner)
            {
                return Joining::Causing;
            }
            const ArabicLetter* letter = FindLetter(character);
            if (letter == nullptr || letter->Final == 0)
            {
                return Joining::None;
            }
            return letter->Initial != 0 ? Joining::Dual : Joining::Right;
        }

        bool JoinsNext(Joining joining)
        {
            return joining == Joining::Dual || joining == Joining::Causing;
        }

        bool JoinsPrevious(Joining joining)
        {
            return joining == Joining::Dual || joining == Joining::Right || joining == Joining::Causing;
        }

        std::u32string Decode(std::string_view text)
        {
            std::u32string result;
            result.reserve(text.size());
            std::size_t index = 0;
            while (index < text.size())
            {
                const auto first = static_cast<unsigned char>(text[index]);
                std::size_t length = 1;
                char32_t character = first;
                if (first >= 0xF0 && first < 0xF8)
                {
                    length = 4;
                    character = first & 0x07U;
                }
                else if (first >= 0xE0)
                {
                    length = 3;
                    character = first & 0x0FU;
                }
                else if (first >= 0xC0)
                {
                    length = 2;
                    character = first & 0x1FU;
                }
                else if (first >= 0x80)
                {
                    length = 0;
                }

                bool valid = length != 0 && index + length <= text.size();
                for (std::size_t offset = 1; valid && offset < length; ++offset)
                {
                    const auto next = static_cast<unsigned char>(text[index + offset]);
                    valid = (next & 0xC0U) == 0x80U;
                    character = (character << 6U) | (next & 0x3FU);
                }
                result += valid ? character : Replacement;
                index += valid ? length : 1;
            }
            return result;
        }

        std::string Encode(std::u32string_view text)
        {
            std::string result;
            result.reserve(text.size() * 2);
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
                else if (character < 0x10000)
                {
                    result += static_cast<char>(0xE0U | (character >> 12U));
                    result += static_cast<char>(0x80U | ((character >> 6U) & 0x3FU));
                    result += static_cast<char>(0x80U | (character & 0x3FU));
                }
                else
                {
                    result += static_cast<char>(0xF0U | (character >> 18U));
                    result += static_cast<char>(0x80U | ((character >> 12U) & 0x3FU));
                    result += static_cast<char>(0x80U | ((character >> 6U) & 0x3FU));
                    result += static_cast<char>(0x80U | (character & 0x3FU));
                }
            }
            return result;
        }

        bool IsRightToLeft(char32_t character)
        {
            return IsInRange(character, 0x0590, 0x08FF) || IsInRange(character, 0xFB1D, 0xFDFF) || IsInRange(character, 0xFE70, 0xFEFF) ||
                   IsInRange(character, 0x10800, 0x10FFF) || IsInRange(character, 0x1E800, 0x1EFFF);
        }

        std::optional<std::size_t> FindNeighbor(std::u32string_view text, std::size_t index, bool forward)
        {
            std::size_t current = index;
            while (forward ? current + 1 < text.size() : current > 0)
            {
                current = forward ? current + 1 : current - 1;
                if (GetJoining(text[current]) != Joining::Transparent)
                {
                    return current;
                }
            }
            return std::nullopt;
        }

        const std::array<char32_t, 3>* FindLamAlef(char32_t alef)
        {
            for (const std::array<char32_t, 3>& entry : LamAlefs)
            {
                if (entry[0] == alef)
                {
                    return &entry;
                }
            }
            return nullptr;
        }

        std::u32string Shape(std::u32string_view text)
        {
            std::u32string result;
            result.reserve(text.size());
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                const char32_t character = text[index];
                const ArabicLetter* letter = FindLetter(character);
                if (letter == nullptr)
                {
                    result += character;
                    continue;
                }

                const std::optional<std::size_t> previous = FindNeighbor(text, index, false);
                const std::optional<std::size_t> next = FindNeighbor(text, index, true);
                const bool joinedBefore = previous && JoinsNext(GetJoining(text[*previous])) && JoinsPrevious(GetJoining(character));

                if (character == Lam && next)
                {
                    if (const std::array<char32_t, 3>* lamAlef = FindLamAlef(text[*next]))
                    {
                        result += joinedBefore ? (*lamAlef)[2] : (*lamAlef)[1];
                        result.append(text.substr(index + 1, *next - index - 1));
                        index = *next;
                        continue;
                    }
                }

                const bool joinedAfter = next && JoinsNext(GetJoining(character)) && JoinsPrevious(GetJoining(text[*next]));
                char32_t form = letter->Isolated;
                if (joinedBefore && joinedAfter)
                {
                    form = letter->Medial;
                }
                else if (joinedBefore)
                {
                    form = letter->Final;
                }
                else if (joinedAfter)
                {
                    form = letter->Initial;
                }
                result += form != 0 ? form : letter->Isolated;
            }
            return result;
        }

        struct BidiObjects
        {
            SBAlgorithmRef Algorithm = nullptr;
            SBParagraphRef Paragraph = nullptr;
            SBLineRef Line = nullptr;

            BidiObjects() = default;
            BidiObjects(const BidiObjects&) = delete;
            BidiObjects& operator=(const BidiObjects&) = delete;
            BidiObjects(BidiObjects&&) = delete;
            BidiObjects& operator=(BidiObjects&&) = delete;

            ~BidiObjects()
            {
                if (Line != nullptr)
                {
                    SBLineRelease(Line);
                }
                if (Paragraph != nullptr)
                {
                    SBParagraphRelease(Paragraph);
                }
                if (Algorithm != nullptr)
                {
                    SBAlgorithmRelease(Algorithm);
                }
            }
        };

        void AppendReversed(std::u32string& result, std::u32string_view text, std::span<const SBBidiType> types, std::size_t first, std::size_t length)
        {
            std::size_t end = first + length;
            while (end > first)
            {
                std::size_t start = end - 1;
                while (start > first && types[start] == SBBidiTypeNSM)
                {
                    --start;
                }
                for (const char32_t character : text.substr(start, end - start))
                {
                    const SBCodepoint mirror = SBCodepointGetMirror(character);
                    result += mirror != 0 ? static_cast<char32_t>(mirror) : character;
                }
                end = start;
            }
        }

        void ReorderParagraph(std::u32string& result, std::u32string_view text, SBLevel level)
        {
            const SBCodepointSequence sequence{.stringEncoding = SBStringEncodingUTF32, .stringBuffer = text.data(), .stringLength = text.size()};
            BidiObjects bidi;
            bidi.Algorithm = SBAlgorithmCreate(&sequence);
            if (bidi.Algorithm == nullptr)
            {
                result.append(text);
                return;
            }
            const std::span<const SBBidiType> types(SBAlgorithmGetBidiTypesPtr(bidi.Algorithm), text.size());

            std::size_t offset = 0;
            while (offset < text.size())
            {
                BidiObjects paragraph;
                paragraph.Paragraph = SBAlgorithmCreateParagraph(bidi.Algorithm, offset, text.size() - offset, level);
                if (paragraph.Paragraph == nullptr)
                {
                    result.append(text.substr(offset));
                    return;
                }
                const SBUInteger length = SBParagraphGetLength(paragraph.Paragraph);
                paragraph.Line = SBParagraphCreateLine(paragraph.Paragraph, offset, length);
                if (paragraph.Line == nullptr)
                {
                    result.append(text.substr(offset));
                    return;
                }

                const SBRun* runs = SBLineGetRunsPtr(paragraph.Line);
                for (const SBRun& run : std::span(runs, SBLineGetRunCount(paragraph.Line)))
                {
                    if ((run.level & 1U) != 0)
                    {
                        AppendReversed(result, text, types, run.offset, run.length);
                    }
                    else
                    {
                        result.append(std::u32string_view(text).substr(run.offset, run.length));
                    }
                }
                offset += length;
            }
        }
    }

    TextDirection GetTextDirection(std::string_view locale)
    {
        const std::string_view language = locale.substr(0, locale.find_first_of("_-"));
        const bool rightToLeft =
            std::ranges::any_of(RightToLeftLanguages, [language](std::string_view name)
                                { return std::ranges::equal(language, name, [](char left, char right) { return (left | 0x20) == right; }); });
        return rightToLeft ? TextDirection::RightToLeft : TextDirection::LeftToRight;
    }

    bool HasRightToLeftText(std::string_view text)
    {
        return std::ranges::any_of(Decode(text), IsRightToLeft);
    }

    std::string ShapeArabic(std::string_view text)
    {
        return Encode(Shape(Decode(text)));
    }

    std::string ReorderForDisplay(std::string_view text, TextDirection direction)
    {
        const std::u32string decoded = Decode(text);
        std::u32string result;
        result.reserve(decoded.size());
        const SBLevel level = direction == TextDirection::RightToLeft ? 1 : 0;
        std::size_t start = 0;
        while (start <= decoded.size())
        {
            const std::size_t end = std::min(decoded.find(U'\n', start), decoded.size());
            ReorderParagraph(result, std::u32string_view(decoded).substr(start, end - start), level);
            if (end == decoded.size())
            {
                break;
            }
            result += U'\n';
            start = end + 1;
        }
        return Encode(result);
    }

    std::string PrepareForDisplay(std::string_view text, TextDirection direction)
    {
        if (direction == TextDirection::LeftToRight && !HasRightToLeftText(text))
        {
            return std::string(text);
        }
        return ReorderForDisplay(ShapeArabic(text), direction);
    }
}
