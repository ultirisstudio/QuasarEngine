#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/io/File.h>
#include <quasar/localization/Localization.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using Quasar::Localization;

namespace
{
    constexpr std::string_view English = R"(language: "English"
menu:
  file: "File"
  new_project: "New Project..."
project:
  created: "Project {name} created in {location}"
  entities:
    one: "{count} entity"
    other: "{count} entities"
  braces: "Use {{name}} for {name}"
)";

    constexpr std::string_view French = R"(language: "Français"
menu:
  file: "Fichier"
project:
  created: "Projet {name} créé dans {location}"
  entities:
    one: "{count} entité"
    other: "{count} entités"
)";

    class Locales
    {
    public:
        Locales()
        {
            Write("en_US.yaml", English);
            Write("fr_FR.yaml", French);
        }

        void Write(std::string_view file, std::string_view text) const { REQUIRE(Quasar::WriteTextFile(m_Folder.GetPath() / file, text)); }

        const std::filesystem::path& GetPath() const { return m_Folder.GetPath(); }

    private:
        TestHelpers::TemporaryFolder m_Folder{"quasar_locales"};
    };
}

TEST_SUITE("Localization")
{
    TEST_CASE("The texts of a locale are found by keys joined by dots")
    {
        const TestHelpers::ScopedLog log;
        const Locales files;
        Localization localization;

        REQUIRE(localization.LoadDirectory(files.GetPath()));
        CHECK(localization.GetLocales() == std::vector<std::string>{"en_US", "fr_FR"});
        CHECK(localization.GetLocale().empty());
        REQUIRE(localization.SetLocale("fr_FR"));

        CHECK(localization.GetLocale() == "fr_FR");
        CHECK(std::string_view(localization.Get("menu.file")) == "Fichier");
        CHECK(localization.GetLanguageName("fr_FR") == "Français");
        CHECK(localization.GetLanguageName("en_US") == "English");
        CHECK(localization.GetLanguageName("de_DE") == "de_DE");
        REQUIRE(localization.SetLocale("en_US"));
        CHECK(std::string_view(localization.Get("menu.new_project")) == "New Project...");
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
    }

    TEST_CASE("A missing text gives its key and logs one warning")
    {
        const TestHelpers::ScopedLog log;
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));

        CHECK(std::string_view(localization.Get("menu.file")) == "menu.file");
        REQUIRE(localization.SetLocale("fr_FR"));
        const char* missing = localization.Get("menu.new_project");
        CHECK(std::string_view(missing) == "menu.new_project");
        CHECK(localization.Get("menu.new_project") == missing);
        CHECK(localization.Format("menu.new_project", {}) == "menu.new_project");
        CHECK(localization.FormatCount("menu.missing", 2) == "menu.missing (count: 2)");
        CHECK(localization.Format("menu.other", {{"path", "C:/Game"}, {"reason", "denied"}}) == "menu.other (path: C:/Game, reason: denied)");
        CHECK(std::string_view(localization.Get("menu")) == "menu");
        CHECK(TestHelpers::CountWarnings("menu.new_project") == 1);
        CHECK(TestHelpers::CountWarnings("menu.missing") == 1);

        REQUIRE(localization.SetLocale("en_US"));
        CHECK(std::string_view(localization.Get("menu.missing")) == "menu.missing");
        CHECK(TestHelpers::CountWarnings("menu.missing") == 2);
    }

    TEST_CASE("The values of the arguments replace their names between braces")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        REQUIRE(localization.SetLocale("fr_FR"));

        CHECK(localization.Format("project.created", {{"location", "C:/Jeux"}, {"name", "Galaxie"}}) == "Projet Galaxie créé dans C:/Jeux");
        CHECK(localization.Format("project.created", {{"name", "{location}"}}) == "Projet {location} créé dans {location}");

        REQUIRE(localization.SetLocale("en_US"));
        CHECK(localization.Format("project.braces", {{"name", "x"}}) == "Use {name} for x");
    }

    TEST_CASE("The form of a text follows the number and the rules of the language")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));

        REQUIRE(localization.SetLocale("fr_FR"));
        CHECK(localization.FormatCount("project.entities", 0) == "0 entité");
        CHECK(localization.FormatCount("project.entities", 1) == "1 entité");
        CHECK(localization.FormatCount("project.entities", 2) == "2 entités");
        CHECK(std::string_view(localization.Get("project.entities")) == "{count} entités");

        REQUIRE(localization.SetLocale("en_US"));
        CHECK(localization.FormatCount("project.entities", 0) == "0 entities");
        CHECK(localization.FormatCount("project.entities", 1) == "1 entity");
        CHECK(localization.FormatCount("project.entities", -1) == "-1 entities");
        CHECK(localization.FormatCount("menu.file", 3) == "File");

        files.Write("ja_JP.yaml", "items:\n  one: \"{count} one\"\n  other: \"{count} items\"\n");
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        REQUIRE(localization.SetLocale("ja_JP"));
        CHECK(localization.FormatCount("items", 1) == "1 items");
    }

    TEST_CASE("A locale is found by its name, or else by its language")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));

        CHECK(localization.FindLocale("fr_FR") == "fr_FR");
        CHECK(localization.FindLocale("fr-fr") == "fr_FR");
        CHECK(localization.FindLocale("fr_CA") == "fr_FR");
        CHECK(localization.FindLocale("FR") == "fr_FR");
        CHECK_FALSE(localization.FindLocale("de_DE").has_value());

        CHECK_FALSE(localization.SetLocale("de_DE"));
        CHECK(localization.GetLocale().empty());
        REQUIRE(localization.SetLocale("fr_BE"));
        CHECK(localization.GetLocale() == "fr_FR");
        CHECK_FALSE(localization.SetLocale("de_DE"));
        CHECK(localization.GetLocale() == "fr_FR");
    }

    TEST_CASE("The locale chosen is the first preferred one that fits, else the fallback, else the first one")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));

        const std::array<std::string, 3> preferred{"de_DE", "fr_CA", "en_US"};
        CHECK(localization.ChooseLocale(preferred, "en_US") == "fr_FR");
        CHECK(localization.ChooseLocale(std::array<std::string, 1>{"de_DE"}, "fr_CA") == "fr_FR");
        CHECK(localization.ChooseLocale({}, "de_DE") == "en_US");
        CHECK(Localization().ChooseLocale(preferred, "en_US").empty());
    }

    TEST_CASE("Loading the files again keeps the locale used, and forgets the files removed")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        REQUIRE(localization.SetLocale("fr_FR"));

        files.Write("fr_FR.yaml", "menu:\n  file: \"Dossier\"\n");
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        CHECK(localization.GetLocale() == "fr_FR");
        CHECK(std::string_view(localization.Get("menu.file")) == "Dossier");

        std::filesystem::remove(files.GetPath() / "fr_FR.yaml");
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        CHECK(localization.GetLocale().empty());
        CHECK(localization.GetLocales() == std::vector<std::string>{"en_US"});
        CHECK(std::string_view(localization.Get("menu.file")) == "menu.file");
    }

    TEST_CASE("The files that are not valid are skipped, and the reason is logged")
    {
        const TestHelpers::ScopedLog log;
        const Locales files;
        files.Write("de_DE.yaml", "menu: [\n");
        files.Write("es_ES.yaml", "- a\n- b\n");
        files.Write("it_IT.yaml", "menu:\n  file: \"File\"\n  list: [a, b]\n  other: \"Altro\"\n");
        files.Write("notes.txt", "not a locale");
        Localization localization;

        REQUIRE(localization.LoadDirectory(files.GetPath()));

        CHECK(localization.GetLocales() == std::vector<std::string>{"en_US", "fr_FR", "it_IT"});
        CHECK(TestHelpers::CountErrors("de_DE.yaml") == 1);
        CHECK(TestHelpers::CountErrors("es_ES.yaml") == 1);
        CHECK(TestHelpers::CountWarnings("menu.list") == 1);
        REQUIRE(localization.SetLocale("it_IT"));
        CHECK(std::string_view(localization.Get("menu.file")) == "File");
        CHECK(std::string_view(localization.Get("menu.other")) == "Altro");
        CHECK_FALSE(localization.LoadDirectory(files.GetPath() / "missing"));
        CHECK(localization.GetLocales().empty());
    }

    TEST_CASE("A label shows the text and keeps the key as the ImGui identifier")
    {
        const Locales files;
        Localization localization;
        REQUIRE(localization.LoadDirectory(files.GetPath()));
        REQUIRE(localization.SetLocale("fr_FR"));

        CHECK(localization.GetLabel("menu.file") == "Fichier###menu.file");
        CHECK(localization.GetLabel("menu.none") == "menu.none###menu.none");
    }

    TEST_CASE("Locales are written with the language in lowercase and the country in uppercase")
    {
        CHECK(Quasar::NormalizeLocale("fr-ca") == "fr_CA");
        CHECK(Quasar::NormalizeLocale("FR_fr") == "fr_FR");
        CHECK(Quasar::NormalizeLocale("EN") == "en");
        CHECK(Quasar::NormalizeLocale("") == "");
    }

    TEST_CASE("The preferred locales of the system are written like the files")
    {
        for (const std::string& locale : Quasar::GetPreferredLocales())
        {
            CAPTURE(locale);
            CHECK(locale == Quasar::NormalizeLocale(locale));
            CHECK_FALSE(locale.empty());
        }
    }
}
