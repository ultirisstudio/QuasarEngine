#include <quasar/localization/Localization.h>

#include <quasar/io/File.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_locale.h>
#include <SDL3/SDL_stdinc.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <system_error>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr std::string_view LanguageKey = "language";
        constexpr std::array<std::string_view, 6> PluralForms{"zero", "one", "two", "few", "many", "other"};

        std::string_view GetLanguage(std::string_view locale)
        {
            return locale.substr(0, locale.find('_'));
        }

        // The languages whose CLDR rule is not "one for 1 only"
        constexpr std::array<std::string_view, 3> ZeroAndOne{"fr", "pt", "hi"};
        constexpr std::array<std::string_view, 6> NoPlural{"ja", "zh", "ko", "th", "vi", "id"};

        struct LocaleListDeleter
        {
            void operator()(SDL_Locale** list) const
            {
                // SDL allocates the list and the locales in one block, which it frees as one pointer
                // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                SDL_free(static_cast<void*>(list));
            }
        };

        bool IsOne(std::string_view language, std::int64_t count)
        {
            if (std::ranges::find(NoPlural, language) != NoPlural.end())
            {
                return false;
            }
            if (std::ranges::find(ZeroAndOne, language) != ZeroAndOne.end())
            {
                return count == 0 || count == 1;
            }
            return count == 1;
        }

        // A key that a constant node does not have gives a node that is not valid, which only IsDefined() can test
        bool IsText(const YAML::Node& node)
        {
            return node.IsDefined() && node.IsScalar();
        }

        bool IsPluralMap(const YAML::Node& node)
        {
            if (!node.IsMap() || !IsText(node["other"]))
            {
                return false;
            }
            return std::ranges::all_of(node, [](const auto& entry)
                                       { return std::ranges::find(PluralForms, entry.first.template as<std::string>()) != PluralForms.end(); });
        }

        // A missing text still shows its values, so that nothing is lost, such as the reason of an error
        std::string WithArguments(const std::string& key, std::span<const TextArgument> arguments)
        {
            if (arguments.empty())
            {
                return key;
            }
            std::string result = key + " (";
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                result += (index == 0 ? "" : ", ") + std::string(arguments[index].Name) + ": " + arguments[index].Value;
            }
            return result + ")";
        }

        void Append(std::string& result, std::string_view text, std::span<const TextArgument> arguments)
        {
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                const char character = text[index];
                if ((character == '{' || character == '}') && index + 1 < text.size() && text[index + 1] == character)
                {
                    result += character;
                    ++index;
                    continue;
                }
                const std::size_t end = character == '{' ? text.find('}', index) : std::string_view::npos;
                if (end == std::string_view::npos)
                {
                    result += character;
                    continue;
                }
                const std::string_view name = text.substr(index + 1, end - index - 1);
                const auto argument = std::ranges::find(arguments, name, &TextArgument::Name);
                if (argument == arguments.end())
                {
                    result += character;
                    continue;
                }
                result += argument->Value;
                index = end;
            }
        }
    }

    void Localization::ReadTexts(Locale& locale, const YAML::Node& root, const std::filesystem::path& file)
    {
        std::vector<std::pair<YAML::Node, std::string>> maps{{root, ""}};
        while (!maps.empty())
        {
            const auto [map, prefix] = std::move(maps.back());
            maps.pop_back();
            for (const auto& entry : map)
            {
                const std::string key = prefix + entry.first.as<std::string>();
                const YAML::Node& value = entry.second;
                if (value.IsScalar())
                {
                    locale.Texts[key] = {.Other = value.Scalar(), .One = std::nullopt};
                }
                else if (IsPluralMap(value))
                {
                    const YAML::Node one = value["one"];
                    locale.Texts[key] = {.Other = value["other"].Scalar(), .One = IsText(one) ? std::optional(one.Scalar()) : std::nullopt};
                }
                else if (value.IsMap())
                {
                    maps.emplace_back(value, key + ".");
                }
                else
                {
                    QUASAR_ENGINE_WARN("The text '{}' of '{}' is not a text or a map, and is skipped", key, PathToUtf8(file));
                }
            }
        }
    }

    bool Localization::LoadDirectory(const std::filesystem::path& directory)
    {
        m_Locales.clear();
        m_LocaleNames.clear();
        m_Missing.clear();
        m_Texts = nullptr;

        std::error_code error;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, error))
        {
            const std::filesystem::path& file = entry.path();
            if (file.extension() != FileExtension || !entry.is_regular_file(error))
            {
                continue;
            }
            const std::optional<std::string> text = ReadTextFile(file);
            if (!text)
            {
                continue;
            }

            YAML::Node root;
            try
            {
                root = YAML::Load(*text);
            }
            catch (const YAML::Exception& exception)
            {
                QUASAR_ENGINE_ERROR("The texts '{}' are not valid YAML and are skipped: {}", PathToUtf8(file), exception.msg);
                continue;
            }
            if (!root.IsMap())
            {
                QUASAR_ENGINE_ERROR("The texts '{}' must be a map of keys and are skipped", PathToUtf8(file));
                continue;
            }

            Locale locale;
            ReadTexts(locale, root, file);
            if (const auto name = locale.Texts.find(LanguageKey); name != locale.Texts.end())
            {
                locale.LanguageName = name->second.Other;
            }
            m_Locales.insert_or_assign(NormalizeLocale(PathToUtf8(file.stem())), std::move(locale));
        }
        if (error)
        {
            QUASAR_ENGINE_ERROR("Cannot read the folder of the texts '{}': {}", PathToUtf8(directory), error.message());
        }

        for (const auto& [name, locale] : m_Locales)
        {
            m_LocaleNames.push_back(name);
        }
        const std::string current = std::exchange(m_Current, {});
        if (!current.empty() && m_Locales.contains(current))
        {
            SetLocale(current);
        }
        return !m_Locales.empty();
    }

    std::string_view Localization::GetLanguageName(std::string_view locale) const
    {
        const auto found = m_Locales.find(locale);
        if (found == m_Locales.end() || found->second.LanguageName.empty())
        {
            return locale;
        }
        return found->second.LanguageName;
    }

    std::optional<std::string> Localization::FindLocale(std::string_view wanted) const
    {
        std::string locale = NormalizeLocale(wanted);
        if (m_Locales.contains(locale))
        {
            return locale;
        }
        const std::string_view language = GetLanguage(locale);
        for (const std::string& name : m_LocaleNames)
        {
            if (GetLanguage(name) == language)
            {
                return name;
            }
        }
        return std::nullopt;
    }

    std::string Localization::ChooseLocale(std::span<const std::string> preferred, std::string_view fallback) const
    {
        for (const std::string& locale : preferred)
        {
            if (std::optional<std::string> found = FindLocale(locale))
            {
                return *found;
            }
        }
        if (std::optional<std::string> found = FindLocale(fallback))
        {
            return *found;
        }
        return m_LocaleNames.empty() ? std::string() : m_LocaleNames.front();
    }

    bool Localization::SetLocale(std::string_view locale)
    {
        const std::optional<std::string> found = FindLocale(locale);
        if (!found)
        {
            return false;
        }
        m_Current = *found;
        m_Texts = &m_Locales.find(m_Current)->second;
        m_Missing.clear();
        return true;
    }

    const Localization::Text* Localization::Find(std::string_view key) const
    {
        if (m_Texts != nullptr)
        {
            if (const auto found = m_Texts->Texts.find(key); found != m_Texts->Texts.end())
            {
                return &found->second;
            }
        }
        return nullptr;
    }

    const std::string& Localization::Missing(std::string_view key) const
    {
        auto found = m_Missing.find(key);
        if (found == m_Missing.end())
        {
            found = m_Missing.emplace(key).first;
            QUASAR_ENGINE_WARN("The locale '{}' has no text for the key '{}'", m_Current, key);
        }
        return *found;
    }

    const char* Localization::Get(std::string_view key) const
    {
        const Text* text = Find(key);
        return text != nullptr ? text->Other.c_str() : Missing(key).c_str();
    }

    std::string Localization::Format(std::string_view key, std::initializer_list<TextArgument> arguments) const
    {
        const Text* text = Find(key);
        if (text == nullptr)
        {
            return WithArguments(Missing(key), std::span(arguments.begin(), arguments.size()));
        }
        std::string result;
        Append(result, text->Other, std::span(arguments.begin(), arguments.size()));
        return result;
    }

    std::string Localization::FormatCount(std::string_view key, std::int64_t count, std::initializer_list<TextArgument> arguments) const
    {
        std::vector<TextArgument> all(arguments);
        all.push_back({.Name = "count", .Value = std::to_string(count)});
        const Text* text = Find(key);
        if (text == nullptr)
        {
            return WithArguments(Missing(key), all);
        }
        const std::string& form = text->One && IsOne(GetLanguage(m_Current), count) ? *text->One : text->Other;
        std::string result;
        Append(result, form, all);
        return result;
    }

    std::string Localization::GetLabel(std::string_view key) const
    {
        return std::string(Get(key)) + "###" + std::string(key);
    }

    std::string NormalizeLocale(std::string_view locale)
    {
        std::string result;
        bool country = false;
        for (const char character : locale)
        {
            if (character == '-' || character == '_')
            {
                result += '_';
                country = true;
                continue;
            }
            const auto byte = static_cast<unsigned char>(character);
            result += static_cast<char>(country ? std::toupper(byte) : std::tolower(byte));
        }
        return result;
    }

    std::vector<std::string> GetPreferredLocales()
    {
        std::vector<std::string> locales;
        int count = 0;
        const std::unique_ptr<SDL_Locale*, LocaleListDeleter> list(SDL_GetPreferredLocales(&count));
        if (list == nullptr || count <= 0)
        {
            return locales;
        }
        for (const SDL_Locale* locale : std::span(list.get(), static_cast<std::size_t>(count)))
        {
            if (locale == nullptr || locale->language == nullptr)
            {
                continue;
            }
            std::string name = locale->language;
            if (locale->country != nullptr)
            {
                name += '_';
                name += locale->country;
            }
            locales.push_back(NormalizeLocale(name));
        }
        return locales;
    }
}
