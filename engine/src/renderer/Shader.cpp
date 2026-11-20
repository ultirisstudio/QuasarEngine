#include <quasar/renderer/Shader.h>

#include <quasar/io/File.h>
#include <quasar/log/Log.h>
#include <spdlog/fmt/std.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
        constexpr std::string_view TypeDirective = "#type";
        constexpr std::string_view IncludeDirective = "#include";
        constexpr std::array<ShaderStage, 2> Stages{ShaderStage::Vertex, ShaderStage::Fragment};

        std::string_view Trim(std::string_view text)
        {
            constexpr std::string_view spaces = " \t\r";
            const std::size_t first = text.find_first_not_of(spaces);
            if (first == std::string_view::npos)
            {
                return {};
            }
            return text.substr(first, text.find_last_not_of(spaces) - first + 1);
        }

        std::optional<std::string_view> NextLine(std::string_view text, std::size_t& position)
        {
            if (position >= text.size())
            {
                return std::nullopt;
            }
            const std::size_t end = text.find('\n', position);
            const std::string_view line = text.substr(position, end == std::string_view::npos ? std::string_view::npos : end - position);
            position = end == std::string_view::npos ? text.size() : end + 1;
            return line;
        }

        bool IsDirective(std::string_view line, std::string_view directive)
        {
            return line.starts_with(directive) && (line.size() == directive.size() || line[directive.size()] == ' ' || line[directive.size()] == '\t');
        }

        std::optional<std::string_view> FindIncludedName(std::string_view line)
        {
            const std::string_view quoted = Trim(line.substr(IncludeDirective.size()));
            if (quoted.size() < 3 || quoted.front() != '"' || quoted.back() != '"')
            {
                return std::nullopt;
            }
            return quoted.substr(1, quoted.size() - 2);
        }

        std::filesystem::path Normalize(const std::filesystem::path& file)
        {
            std::error_code error;
            const std::filesystem::path normalized = std::filesystem::weakly_canonical(file, error);
            return error ? file.lexically_normal() : normalized;
        }

        struct IncludedFile
        {
            std::filesystem::path File;
            std::string Location;
            std::string Text;
            std::size_t Position = 0;
            std::size_t Line = 0;
        };

        std::optional<std::string> ExpandIncludes(const std::string& name, const std::filesystem::path& file, const ShaderSource& source)
        {
            std::string expanded;
            std::vector<std::filesystem::path> included;
            std::vector<IncludedFile> files;
            files.push_back({.File = Normalize(file), .Location = std::string("the ") + GetShaderStageName(source.Stage) + " stage", .Text = source.Code});
            while (!files.empty())
            {
                IncludedFile& current = files.back();
                const std::optional<std::string_view> line = NextLine(current.Text, current.Position);
                if (!line)
                {
                    files.pop_back();
                    if (!files.empty())
                    {
                        expanded.append("#line " + std::to_string(files.back().Line + 1) + "\n");
                    }
                    continue;
                }
                ++current.Line;

                const std::string_view trimmed = Trim(*line);
                if (!IsDirective(trimmed, IncludeDirective))
                {
                    expanded.append(*line);
                    expanded.push_back('\n');
                    continue;
                }

                const std::optional<std::string_view> includedName = FindIncludedName(trimmed);
                if (!includedName)
                {
                    QUASAR_ENGINE_ERROR("The shader '{}' has an {} without a quoted file name, at line {} of {}", name, IncludeDirective, current.Line,
                                        current.Location);
                    return std::nullopt;
                }
                const std::filesystem::path includedFile = Normalize(current.File.parent_path() / *includedName);
                if (std::ranges::any_of(files, [&includedFile](const IncludedFile& open) { return open.File == includedFile; }))
                {
                    QUASAR_ENGINE_ERROR("The shader '{}' includes '{}' from itself, at line {} of {}", name, includedFile, current.Line, current.Location);
                    return std::nullopt;
                }
                if (std::ranges::find(included, includedFile) != included.end())
                {
                    expanded.push_back('\n');
                    continue;
                }

                std::optional<std::string> text = ReadTextFile(includedFile);
                if (!text)
                {
                    QUASAR_ENGINE_ERROR("The shader '{}' includes '{}', which cannot be read, at line {} of {}", name, includedFile, current.Line,
                                        current.Location);
                    return std::nullopt;
                }
                included.push_back(includedFile);
                expanded.append("#line 1\n");
                files.push_back({.File = includedFile, .Location = "'" + includedFile.generic_string() + "'", .Text = std::move(*text)});
            }
            return expanded;
        }

        std::optional<ShaderStage> FindStage(std::string_view name)
        {
            for (const ShaderStage stage : Stages)
            {
                if (name == GetShaderStageName(stage))
                {
                    return stage;
                }
            }
            return std::nullopt;
        }
    }

    const char* GetShaderStageName(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:
                return "vertex";
            case ShaderStage::Fragment:
                return "fragment";
        }
        return "unknown";
    }

    std::optional<ShaderSpecification> ShaderSpecification::Parse(std::string name, std::string_view text)
    {
        ShaderSpecification specification{.Name = std::move(name), .Sources = {}};
        std::size_t lineNumber = 0;
        std::size_t position = 0;
        while (const std::optional<std::string_view> line = NextLine(text, position))
        {
            const std::string_view trimmed = Trim(*line);
            ++lineNumber;

            if (IsDirective(trimmed, TypeDirective))
            {
                const std::string_view stageName = Trim(trimmed.substr(TypeDirective.size()));
                const std::optional<ShaderStage> stage = FindStage(stageName);
                if (!stage)
                {
                    QUASAR_ENGINE_ERROR("Unknown shader stage '{}' at line {} of the shader '{}'", stageName, lineNumber, specification.Name);
                    return std::nullopt;
                }
                specification.Sources.push_back({.Stage = *stage, .Code = {}});
            }
            else if (!specification.Sources.empty())
            {
                std::string& code = specification.Sources.back().Code;
                code.append(*line);
                code.push_back('\n');
            }
            else if (!trimmed.empty())
            {
                QUASAR_ENGINE_ERROR("The shader '{}' has code before its first {} line, at line {}", specification.Name, TypeDirective, lineNumber);
                return std::nullopt;
            }
        }

        if (specification.Sources.empty())
        {
            QUASAR_ENGINE_ERROR("The shader '{}' has no {} line", specification.Name, TypeDirective);
            return std::nullopt;
        }
        return specification;
    }

    std::optional<ShaderSpecification> ShaderSpecification::FromFile(const std::filesystem::path& file)
    {
        const std::optional<std::string> text = ReadTextFile(file);
        if (!text)
        {
            return std::nullopt;
        }
        std::optional<ShaderSpecification> specification = Parse(file.stem().generic_string(), *text);
        if (!specification)
        {
            return std::nullopt;
        }
        for (ShaderSource& source : specification->Sources)
        {
            std::optional<std::string> code = ExpandIncludes(specification->Name, file, source);
            if (!code)
            {
                return std::nullopt;
            }
            source.Code = std::move(*code);
        }
        return specification;
    }

    Shader::Shader(std::string name) : m_Name(std::move(name)) {}

    bool Shader::IsValid(const ShaderSpecification& specification)
    {
        for (const ShaderStage stage : Stages)
        {
            const auto count = std::ranges::count_if(specification.Sources, [stage](const ShaderSource& source) { return source.Stage == stage; });
            if (count > 1)
            {
                QUASAR_ENGINE_ERROR("The shader '{}' has more than one {} stage", specification.Name, GetShaderStageName(stage));
                return false;
            }
            if (count == 0 && stage == ShaderStage::Vertex)
            {
                QUASAR_ENGINE_ERROR("The shader '{}' has no vertex stage", specification.Name);
                return false;
            }
        }
        return true;
    }
}
