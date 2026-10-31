#include <platform/SystemDirectories.h>

#include <quasar/core/Log.h>

#include <pwd.h>
#include <unistd.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <vector>

namespace Quasar::Platform
{
    namespace
    {
        std::optional<std::filesystem::path> GetHomeFromUserDatabase()
        {
            const long suggestedSize = sysconf(_SC_GETPW_R_SIZE_MAX);
            std::vector<char> buffer(suggestedSize > 0 ? static_cast<std::size_t>(suggestedSize) : 16384);

            passwd entry{};
            passwd* found = nullptr;
            if (getpwuid_r(getuid(), &entry, buffer.data(), buffer.size(), &found) != 0 || found == nullptr ||
                found->pw_dir == nullptr)
            {
                return std::nullopt;
            }

            return std::filesystem::path(found->pw_dir);
        }
    }

    std::optional<std::filesystem::path> GetSystemConfigDirectory()
    {
        if (const char* configHome = std::getenv("XDG_CONFIG_HOME"); configHome != nullptr)
        {
            std::filesystem::path path(configHome);
            if (path.is_absolute())
            {
                return path;
            }
        }

        if (const char* home = std::getenv("HOME"); home != nullptr)
        {
            const std::filesystem::path path(home);
            if (path.is_absolute())
            {
                return path / ".config";
            }
        }

        if (std::optional<std::filesystem::path> home = GetHomeFromUserDatabase(); home && home->is_absolute())
        {
            return *home / ".config";
        }

        QUASAR_ENGINE_ERROR(
            "Cannot find the config folder: XDG_CONFIG_HOME and HOME are not set, and the user has no home folder");
        return std::nullopt;
    }
}
