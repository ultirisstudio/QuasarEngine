#include "helpers/EditorLocale.h"
#include "helpers/LogHelpers.h"

#include <EditorText.h>

#include <quasar/localization/Localization.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using Quasar::Editor::GetLocalization;
using Quasar::Editor::LoadLocales;
using Quasar::Editor::Tr;
using Quasar::Editor::TrLabel;

TEST_SUITE("EditorText")
{
    TEST_CASE("The editor has its texts in English and in French, which are read without errors")
    {
        const TestHelpers::ScopedLog log;
        TestHelpers::UseEditorLocale("fr_FR");

        const std::vector<std::string>& locales = GetLocalization().GetLocales();
        CHECK(std::ranges::find(locales, "en_US") != locales.end());
        CHECK(std::ranges::find(locales, "fr_FR") != locales.end());
        CHECK(GetLocalization().GetLanguageName("fr_FR") == "Français");
        CHECK(GetLocalization().GetLanguageName("en_US") == "English");
        CHECK(std::string_view(Tr("menu.file")) == "Fichier");
        CHECK(TrLabel("menu.file") == "Fichier###menu.file");

        TestHelpers::UseEditorLocale("en_US");
        CHECK(std::string_view(Tr("menu.file")) == "File");
        CHECK(Tr("gamepad.label", {{"number", "1"}, {"name", "Pad"}}) == "Gamepad 1: Pad");
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
    }

    TEST_CASE("The editor uses the locale saved, or else one of the system, or else English")
    {
        LoadLocales("fr_CA");
        CHECK(GetLocalization().GetLocale() == "fr_FR");

        LoadLocales("de_DE");
        const std::string chosen = GetLocalization().GetLocale();
        CHECK(chosen == GetLocalization().ChooseLocale(Quasar::GetPreferredLocales(), Quasar::Editor::DefaultLocale));
        CHECK_FALSE(chosen.empty());

        LoadLocales("");
        CHECK(GetLocalization().GetLocale() == chosen);
        TestHelpers::UseEditorLocale();
    }
}
