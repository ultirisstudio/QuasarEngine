#include <quasar/io/Path.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>

namespace Quasar
{
    namespace
    {
        constexpr std::string_view ForbiddenCharacters = "<>:\"/\\|?*";
        constexpr unsigned char FirstPrintableCharacter = 0x20U;
        constexpr std::array<std::string_view, 22> ReservedNames{"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                                                 "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

        bool IsReservedName(std::string_view name)
        {
            const std::string_view stem = name.substr(0, name.find('.'));
            return std::ranges::any_of(ReservedNames,
                                       [stem](std::string_view reserved)
                                       {
                                           return std::ranges::equal(
                                               stem, reserved, [](char left, char right)
                                               { return std::toupper(static_cast<unsigned char>(left)) == static_cast<unsigned char>(right); });
                                       });
        }

        std::filesystem::path WithForwardSlashes(const std::filesystem::path& path)
        {
            std::string text = path.generic_string();
            std::replace(text.begin(), text.end(), '\\', '/');
            return {text};
        }
    }

    std::string NormalizePath(const std::filesystem::path& path)
    {
        std::string normalized = WithForwardSlashes(path).lexically_normal().generic_string();

        const bool isRoot = normalized == "/" || (normalized.size() == 3 && normalized[1] == ':');
        if (normalized.size() > 1 && normalized.back() == '/' && !isRoot)
        {
            normalized.pop_back();
        }
        return normalized;
    }

    std::optional<std::string> MakeRelativePath(const std::filesystem::path& path, const std::filesystem::path& root)
    {
        std::error_code pathError;
        std::error_code rootError;
        const std::filesystem::path absolutePath = std::filesystem::absolute(WithForwardSlashes(path), pathError);
        const std::filesystem::path absoluteRoot = std::filesystem::absolute(WithForwardSlashes(root), rootError);
        if (pathError || rootError)
        {
            return std::nullopt;
        }

        const std::filesystem::path relative = absolutePath.lexically_normal().lexically_relative(absoluteRoot.lexically_normal());
        if (relative.empty() || *relative.begin() == "..")
        {
            return std::nullopt;
        }

        return NormalizePath(relative);
    }

    std::filesystem::path PathFromUtf8(std::string_view text)
    {
        std::u8string utf8(text.size(), u8'\0');
        std::ranges::transform(text, utf8.begin(), [](char character) { return static_cast<char8_t>(character); });
        return {utf8};
    }

    std::string PathToUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        std::string text(utf8.size(), '\0');
        std::ranges::transform(utf8, text.begin(), [](char8_t character) { return static_cast<char>(character); });
        return text;
    }

    bool IsValidFileName(std::string_view name)
    {
        if (name.empty() || name == "." || name == ".." || name.back() == ' ' || name.back() == '.')
        {
            return false;
        }
        if (name.find_first_of(ForbiddenCharacters) != std::string_view::npos || IsReservedName(name))
        {
            return false;
        }
        return std::ranges::none_of(name, [](char character) { return static_cast<unsigned char>(character) < FirstPrintableCharacter; });
    }

    std::filesystem::path FindExistingDirectory(std::filesystem::path path)
    {
        std::error_code error;
        while (!path.empty() && !std::filesystem::is_directory(path, error))
        {
            std::filesystem::path parent = path.parent_path();
            if (parent == path)
            {
                return {};
            }
            path = std::move(parent);
        }
        return path;
    }
}
