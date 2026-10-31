#include <platform/SystemDirectories.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <knownfolders.h>
#include <shlobj.h>

#include <quasar/core/Log.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>

namespace Quasar::Platform
{
    std::optional<std::filesystem::path> GetSystemConfigDirectory()
    {
        PWSTR text = nullptr;
        const HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &text);
        const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> owner(text, &CoTaskMemFree);

        if (FAILED(result))
        {
            QUASAR_ENGINE_ERROR("Cannot find the application data folder: error {:#010x}",
                                static_cast<std::uint32_t>(result));
            return std::nullopt;
        }

        return std::filesystem::path(text);
    }
}
