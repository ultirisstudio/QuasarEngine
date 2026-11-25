#pragma once

#include <quasar/localization/Localization.h>

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    inline constexpr std::string_view DefaultLocale = "en_US";

    // The texts of the editor, shared by all its windows
    Localization& GetLocalization();

    std::filesystem::path GetLocalesDirectory();

    // Reads the locales and uses the one saved, else the one of the system, else DefaultLocale
    void LoadLocales(std::string_view saved);

    const char* Tr(std::string_view key);

    std::string Tr(std::string_view key, std::initializer_list<TextArgument> arguments);

    std::string TrCount(std::string_view key, std::int64_t count, std::initializer_list<TextArgument> arguments = {});

    // For every window, popup, menu and button: ImGui identifies them by the key, whatever the language
    std::string TrLabel(std::string_view key);
}
