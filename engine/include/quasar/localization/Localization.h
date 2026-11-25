#pragma once

/// \file
/// \brief The texts of an interface in several languages, read from one YAML file per locale.
/// \author Kurogenshi
/// \date 2026-11-25

#include <quasar/localization/TextLayout.h>

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace YAML
{
    class Node;
}

namespace Quasar
{
    /// @brief A value put in a text in place of `{Name}`, for example `{"name", projectName}`.
    struct TextArgument
    {
        std::string_view Name; ///< The name between the braces, without them.
        std::string Value;     ///< The text that replaces the braces and the name.
    };

    /// @brief The texts of an interface, found by key in the language of the user.
    ///
    /// Each locale has a YAML file named after it, such as `en_US.yaml` or `fr_FR.yaml`: the language in lowercase, an
    /// underscore and the country in uppercase. The maps of the file give the keys, joined by dots:
    ///
    /// ```yaml
    /// language: "Français"
    /// menu:
    ///   file: "Fichier"
    /// project:
    ///   created: "Projet {name} créé"
    ///   entities:
    ///     one: "{count} entité"
    ///     other: "{count} entités"
    /// ```
    ///
    /// Here `menu.file` gives "Fichier". The `language` key holds the name of the language, written in it. A map of
    /// `one` and `other` is a text that depends on a number, see FormatCount().
    ///
    /// The texts of a language written from right to left, such as Arabic in `ar_SY.yaml`, are given ready to draw from left
    /// to right, as ImGui does: shaped and in drawing order, see PrepareForDisplay(). The values of the arguments are
    /// placed first. The name of each language is ready to draw too.
    ///
    /// A key missing in the locale gives the key itself, followed by the values of its arguments, so that the missing text
    /// is seen without losing them, and a warning is logged once.
    /// The texts stay valid until the locale changes or the files are loaded again. A Localization is used from one
    /// thread.
    class Localization
    {
    public:
        /// The extension of the files of the locales.
        static constexpr std::string_view FileExtension = ".yaml";

        /// @brief Reads the files of the locales in a folder, replacing the ones read before.
        ///
        /// A file that cannot be read or is not valid is skipped, and the reason is logged as an engine error. The
        /// locale stays chosen if its file is still there.
        /// @param directory The folder of the files.
        /// @return True if at least one locale was read.
        bool LoadDirectory(const std::filesystem::path& directory);

        /// @brief Returns the names of the locales read, in alphabetical order, such as "en_US" and "fr_FR".
        const std::vector<std::string>& GetLocales() const { return m_LocaleNames; }

        /// @brief Returns the name of the language of a locale, written in it, such as "Français", or the name of the
        /// locale if its file has no `language` key or if it was not read.
        std::string_view GetLanguageName(std::string_view locale) const;

        /// @brief Finds the locale read that fits a wanted one: the same locale, or else one of the same language.
        /// @param wanted The locale, such as "fr_CA", "fr-CA" or "fr". The case does not matter.
        /// @return The name of the locale read, such as "fr_FR", or no value if no locale of this language was read.
        std::optional<std::string> FindLocale(std::string_view wanted) const;

        /// @brief Chooses the locale that fits the first of the preferred ones, else the fallback, else the first
        /// locale read. See FindLocale().
        /// @return The name of the locale, or an empty string if no locale was read.
        std::string ChooseLocale(std::span<const std::string> preferred, std::string_view fallback) const;

        /// @brief Uses the texts of a locale. See FindLocale().
        /// @return False, and the locale does not change, if no locale read fits.
        bool SetLocale(std::string_view locale);

        /// @brief Returns the name of the locale used, or an empty string if none is used yet.
        const std::string& GetLocale() const { return m_Current; }

        /// @brief Returns the direction in which the language of the locale used is written.
        TextDirection GetDirection() const { return m_Texts != nullptr ? m_Texts->Direction : TextDirection::LeftToRight; }

        /// @brief Returns true if the locale used has a text for a key, for example to use a default text in place of a
        /// text that a translation does not have yet, without logging a warning.
        bool Contains(std::string_view key) const { return Find(key) != nullptr; }

        /// @brief Returns the text of a key, or the key itself if the locale has no text for it.
        const char* Get(std::string_view key) const;

        /// @brief Returns the text of a key with each `{name}` replaced by the value of its argument.
        ///
        /// `{{` and `}}` give a brace. A name without an argument stays as it is.
        std::string Format(std::string_view key, std::initializer_list<TextArgument> arguments) const;

        /// @brief Returns the form of a text that fits a number, with `{count}` replaced by the number.
        ///
        /// The key is a map of `one` and `other`. The language chooses the form: in French 0 and 1 take `one`, in
        /// English only 1 does, and in Japanese, Chinese or Korean every number takes `other`. A key that is a single
        /// text is used for every number.
        std::string FormatCount(std::string_view key, std::int64_t count, std::initializer_list<TextArgument> arguments = {}) const;

        /// @brief Returns the text of a key followed by `###` and the key, so that ImGui identifies a window, a popup or a
        /// menu by its key, whatever the language of its title.
        std::string GetLabel(std::string_view key) const;

    private:
        struct Text
        {
            std::string Other;
            std::optional<std::string> One;
            std::string Display;
        };

        struct Locale
        {
            std::string LanguageName;
            TextDirection Direction = TextDirection::LeftToRight;
            std::map<std::string, Text, std::less<>> Texts;
        };

        static void ReadTexts(Locale& locale, const YAML::Node& root, const std::filesystem::path& file);
        const Text* Find(std::string_view key) const;
        std::string ForDisplay(std::string text) const;
        const std::string& Missing(std::string_view key) const;

        std::map<std::string, Locale, std::less<>> m_Locales;
        std::vector<std::string> m_LocaleNames;
        std::string m_Current;
        const Locale* m_Texts = nullptr;
        mutable std::set<std::string, std::less<>> m_Missing;
    };

    /// @brief Writes a locale the way the files are named: "fr-ca" and "fr_CA" give "fr_CA", and "FR" gives "fr".
    std::string NormalizeLocale(std::string_view locale);

    /// @brief Returns the locales chosen by the user in the settings of the system, the preferred first, such as
    /// {"fr_FR", "en_US"}. Some systems only give the language, such as "fr".
    std::vector<std::string> GetPreferredLocales();
}
