#include "EditorText.h"

#include <quasar/io/EngineResources.h>

#include <string>
#include <vector>

namespace Quasar::Editor
{
    Localization& GetLocalization()
    {
        static Localization localization;
        return localization;
    }

    std::filesystem::path GetLocalesDirectory()
    {
        return GetResourcesDirectory() / "editor" / "locales";
    }

    void LoadLocales(std::string_view saved)
    {
        Localization& localization = GetLocalization();
        localization.LoadDirectory(GetLocalesDirectory());
        if (saved.empty() || !localization.SetLocale(saved))
        {
            const std::vector<std::string> preferred = GetPreferredLocales();
            localization.SetLocale(localization.ChooseLocale(preferred, DefaultLocale));
        }
    }

    const char* Tr(std::string_view key)
    {
        return GetLocalization().Get(key);
    }

    std::string Tr(std::string_view key, std::initializer_list<TextArgument> arguments)
    {
        return GetLocalization().Format(key, arguments);
    }

    std::string TrCount(std::string_view key, std::int64_t count, std::initializer_list<TextArgument> arguments)
    {
        return GetLocalization().FormatCount(key, count, arguments);
    }

    std::string TrLabel(std::string_view key)
    {
        return GetLocalization().GetLabel(key);
    }
}
