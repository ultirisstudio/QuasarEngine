#pragma once

#include <EditorText.h>

#include <doctest/doctest.h>

#include <string_view>

namespace TestHelpers
{
    // The texts of the editor in a locale, so that a test can check them whatever the language of the system
    inline void UseEditorLocale(std::string_view locale = Quasar::Editor::DefaultLocale)
    {
        Quasar::Editor::LoadLocales(locale);
        REQUIRE(Quasar::Editor::GetLocalization().GetLocale() == locale);
    }
}
