#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/io/File.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

TEST_CASE("The files that a shader file includes are inserted once in each stage, from the folder of the file that includes them")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder;
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "include" / "A.glsl", "#include \"B.glsl\"\nfloat a;\n"));
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "include" / "B.glsl", "float b;"));
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "Shader.glsl", "#type vertex\n"
                                                                    "#version 450 core\n"
                                                                    "#include \"include/A.glsl\"\n"
                                                                    "#include \"include/../include/A.glsl\"\n"
                                                                    "void main() {}\n"
                                                                    "#type fragment\n"
                                                                    "#version 450 core\n"
                                                                    "  #include\t \"include/B.glsl\"  \n"
                                                                    "#includes are kept\n"));

    const std::optional<ShaderSpecification> specification = ShaderSpecification::FromFile(folder.GetPath() / "Shader.glsl");

    REQUIRE(specification.has_value());
    REQUIRE(specification->Sources.size() == 2);
    CHECK(specification->Sources[0].Code == "#version 450 core\n#line 1\n#line 1\nfloat b;\n#line 2\nfloat a;\n#line 3\n\nvoid main() {}\n");
    CHECK(specification->Sources[1].Code == "#version 450 core\n#line 1\nfloat b;\n#line 3\n#includes are kept\n");
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A shader file that includes a missing file, includes a file from itself or names no file is refused")
{
    const TestHelpers::TemporaryFolder folder;
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "A.glsl", "#include \"B.glsl\"\n"));
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "B.glsl", "#include \"A.glsl\"\n"));
    const std::array<std::pair<std::string_view, std::string_view>, 7> refused{{
        {"#include \"Missing.glsl\"", "cannot be read, at line 2 of the vertex stage"},
        {"#include \"A.glsl\"", "from itself, at line 1 of '"},
        {"#include \"Shader.glsl\"", "from itself, at line 2 of the vertex stage"},
        {"#include A.glsl", "without a quoted file name"},
        {"#include \"\"", "without a quoted file name"},
        {"#include \"A.glsl", "without a quoted file name"},
        {"#include A.glsl\"", "without a quoted file name"},
    }};

    for (const auto& [include, error] : refused)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(include);
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "Shader.glsl", std::string("#type vertex\n#version 450 core\n") + std::string(include) + "\n"));
        CHECK_FALSE(ShaderSpecification::FromFile(folder.GetPath() / "Shader.glsl").has_value());
        CHECK(TestHelpers::CountErrors(error) == 1);
    }
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
