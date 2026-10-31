#include <quasar/io/Path.h>

#include <algorithm>
#include <system_error>

namespace Quasar
{
    namespace
    {
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

        const std::filesystem::path relative =
            absolutePath.lexically_normal().lexically_relative(absoluteRoot.lexically_normal());
        if (relative.empty() || *relative.begin() == "..")
        {
            return std::nullopt;
        }

        return NormalizePath(relative);
    }
}
