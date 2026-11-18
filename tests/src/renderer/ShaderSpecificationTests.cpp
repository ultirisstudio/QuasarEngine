#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <array>
#include <optional>
#include <string_view>

using Quasar::ShaderSpecification;
using Quasar::ShaderStage;

TEST_CASE("A shader text is split into its stages at each #type line")
{
    const TestHelpers::ScopedLog log;
    constexpr std::string_view text = "\r\n"
                                      "#type vertex\r\n"
                                      "#version 450 core\r\n"
                                      "void main() {}\r\n"
                                      "  #type   fragment  \n"
                                      "#version 450 core\n"
                                      "void main() {}";

    const std::optional<ShaderSpecification> specification = ShaderSpecification::Parse("Test", text);

    REQUIRE(specification.has_value());
    CHECK(specification->Name == "Test");
    REQUIRE(specification->Sources.size() == 2);
    CHECK(specification->Sources[0].Stage == ShaderStage::Vertex);
    CHECK(specification->Sources[0].Code == "#version 450 core\r\nvoid main() {}\r\n");
    CHECK(specification->Sources[1].Stage == ShaderStage::Fragment);
    CHECK(specification->Sources[1].Code == "#version 450 core\nvoid main() {}\n");
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A shader text with an unknown stage, code before its first #type or no #type is refused")
{
    constexpr std::array<std::string_view, 5> texts{
        "#type geometry\nvoid main() {}", "#version 450 core\n#type vertex\nvoid main() {}", "#typevertex\nvoid main() {}", "#type\nvoid main() {}", "",
    };

    for (const std::string_view text : texts)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(text);
        CHECK_FALSE(ShaderSpecification::Parse("Test", text).has_value());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
}

TEST_CASE("A shader file is read and named after the file, without its extension")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFile file("FlatColor.glsl", "#type vertex\nvoid main() {}\n");

    const std::optional<ShaderSpecification> specification = ShaderSpecification::FromFile(file.GetPath());

    REQUIRE(specification.has_value());
    CHECK(specification->Name == "FlatColor");
    REQUIRE(specification->Sources.size() == 1);
    CHECK(specification->Sources[0].Code == "void main() {}\n");
}

TEST_CASE("A shader file that cannot be read is refused")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder;

    CHECK_FALSE(ShaderSpecification::FromFile(folder.GetPath() / "Missing.glsl").has_value());
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A shader needs a vertex stage, and each stage only once")
{
    const TestHelpers::ScopedLog log;
    const Quasar::ShaderSource vertex{.Stage = ShaderStage::Vertex, .Code = "void main() {}"};
    const Quasar::ShaderSource fragment{.Stage = ShaderStage::Fragment, .Code = "void main() {}"};

    CHECK(Quasar::Shader::IsValid({.Name = "Vertex", .Sources = {vertex}}));
    CHECK(Quasar::Shader::IsValid({.Name = "Both", .Sources = {fragment, vertex}}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    CHECK_FALSE(Quasar::Shader::IsValid({.Name = "Fragment", .Sources = {fragment}}));
    CHECK_FALSE(Quasar::Shader::IsValid({.Name = "Twice", .Sources = {vertex, fragment, fragment}}));
    CHECK_FALSE(Quasar::Shader::IsValid({.Name = "Empty", .Sources = {}}));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
