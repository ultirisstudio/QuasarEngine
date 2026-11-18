#include <quasar/renderer/Shader.h>

#include <quasar/io/File.h>
#include <quasar/log/Log.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr std::string_view TypeDirective = "#type";
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

        bool IsTypeDirective(std::string_view line)
        {
            return line.starts_with(TypeDirective) &&
                   (line.size() == TypeDirective.size() || line[TypeDirective.size()] == ' ' || line[TypeDirective.size()] == '\t');
        }

        std::optional<ShaderStage> FindStage(std::string_view name)
        {
            const auto* stage = std::ranges::find_if(Stages, [name](ShaderStage candidate) { return name == GetShaderStageName(candidate); });
            return stage != Stages.end() ? std::optional<ShaderStage>(*stage) : std::nullopt;
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
        while (position < text.size())
        {
            const std::size_t end = text.find('\n', position);
            const std::string_view line = text.substr(position, end == std::string_view::npos ? std::string_view::npos : end - position);
            const std::string_view trimmed = Trim(line);
            ++lineNumber;

            if (IsTypeDirective(trimmed))
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
                code.append(line);
                code.push_back('\n');
            }
            else if (!trimmed.empty())
            {
                QUASAR_ENGINE_ERROR("The shader '{}' has code before its first {} line, at line {}", specification.Name, TypeDirective, lineNumber);
                return std::nullopt;
            }

            if (end == std::string_view::npos)
            {
                break;
            }
            position = end + 1;
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
        return Parse(file.stem().generic_string(), *text);
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
