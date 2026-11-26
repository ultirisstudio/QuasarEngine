#include "helpers/EditorLocale.h"
#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <EditorText.h>
#include <EditorUI.h>

#include <quasar/io/File.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

using Quasar::Editor::UI::JoinList;
using Quasar::Editor::UI::WrapRightToLeft;

namespace
{
    float CountCharacters(std::string_view text)
    {
        return static_cast<float>(text.size());
    }
}

TEST_SUITE("EditorUI")
{
    TEST_CASE("A text written from right to left is wrapped from its end, which is read first")
    {
        CHECK(WrapRightToLeft("aa bb cc", 5.0f, CountCharacters) == std::vector<std::string>{"bb cc", "aa"});
        CHECK(WrapRightToLeft("aa bb cc", 8.0f, CountCharacters) == std::vector<std::string>{"aa bb cc"});
        CHECK(WrapRightToLeft("aa bb cc", 1.0f, CountCharacters) == std::vector<std::string>{"cc", "bb", "aa"});
    }

    TEST_CASE("Each line of a text written from right to left is wrapped on its own, and a long word stays whole")
    {
        CHECK(WrapRightToLeft("aa\nbb cc", 100.0f, CountCharacters) == std::vector<std::string>{"aa", "bb cc"});
        CHECK(WrapRightToLeft("abcdefgh", 3.0f, CountCharacters) == std::vector<std::string>{"abcdefgh"});
        CHECK(WrapRightToLeft("", 3.0f, CountCharacters) == std::vector<std::string>{""});
        CHECK(WrapRightToLeft("aa\n", 3.0f, CountCharacters) == std::vector<std::string>{"aa", ""});
    }

    TEST_CASE("A list uses the separator of the language, or a comma when the language has none yet")
    {
        const TestHelpers::ScopedLog log;
        const TestHelpers::TemporaryFolder folder("quasar_list_separator");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "fr_FR.yaml", "list_separator: \" ; \"\n"));
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "ar_SY.yaml", "list_separator: \"\u060C \"\n"));
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "he_IL.yaml", "language: \"\u05E2\u05D1\u05E8\u05D9\u05EA\"\n"));
        const std::array<const char*, 3> items{"A", "B", "C"};

        TestHelpers::UseEditorLocale();
        CHECK(JoinList(items) == "A, B, C");
        CHECK(JoinList(std::array<const char*, 1>{"A"}) == "A");
        CHECK(JoinList({}).empty());

        Quasar::Localization& localization = Quasar::Editor::GetLocalization();
        REQUIRE(localization.LoadDirectory(folder.GetPath()));
        REQUIRE(localization.SetLocale("fr_FR"));
        CHECK(JoinList(items) == "A ; B ; C");
        REQUIRE(localization.SetLocale("ar_SY"));
        CHECK(JoinList(items) == "C \u060CB \u060CA");
        REQUIRE(localization.SetLocale("he_IL"));
        CHECK(JoinList(items) == "C ,B ,A");

        TestHelpers::UseEditorLocale();
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
    }
}
