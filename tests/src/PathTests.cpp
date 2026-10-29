#include <quasar/io/Path.h>

#include <doctest/doctest.h>

#include <array>
#include <filesystem>
#include <optional>
#include <ostream> // CAPTURE prints a std::string_view with operator<<, which MSVC only declares without this header
#include <string>
#include <string_view>

TEST_CASE("The same file gives the same normalized path with both separators")
{
    const auto spellings = std::to_array<std::string_view>({
        "Assets/Models/cube.obj",
        R"(Assets\Models\cube.obj)",
        R"(Assets\Textures\..\Models\cube.obj)",
        "./Assets//Models/./cube.obj",
        R"(Assets/Models\cube.obj)",
    });

    for (const std::string_view spelling : spellings)
    {
        CAPTURE(spelling);
        CHECK(Quasar::NormalizePath(spelling) == "Assets/Models/cube.obj");
    }
}

TEST_CASE("A normalized directory has no separator at the end")
{
    CHECK(Quasar::NormalizePath("Assets/Models/") == "Assets/Models");
    CHECK(Quasar::NormalizePath("Assets\\Models\\") == "Assets/Models");
}

TEST_CASE("Normalizing keeps what cannot be removed")
{
    CHECK(Quasar::NormalizePath("../Shared/shader.glsl") == "../Shared/shader.glsl");
    CHECK(Quasar::NormalizePath("/") == "/");
    CHECK(Quasar::NormalizePath("Assets/Cube.OBJ") == "Assets/Cube.OBJ");
    CHECK(Quasar::NormalizePath("").empty());
}

TEST_CASE("A path is made relative to a root folder")
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "QuasarProject";

    const std::optional<std::string> relative = Quasar::MakeRelativePath(root / "Assets" / "cube.obj", root);

    REQUIRE(relative.has_value());
    CHECK(*relative == "Assets/cube.obj");
}

TEST_CASE("A relative path gives the same result when both separators are mixed")
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "QuasarProject";
    const std::string mixed = root.generic_string() + R"(\Assets\Textures\..\cube.obj)";

    const std::optional<std::string> relative = Quasar::MakeRelativePath(mixed, root);

    REQUIRE(relative.has_value());
    CHECK(*relative == "Assets/cube.obj");
}

TEST_CASE("The root folder itself is made relative as a dot")
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "QuasarProject";

    const std::optional<std::string> relative = Quasar::MakeRelativePath(root, root);

    REQUIRE(relative.has_value());
    CHECK(*relative == ".");
}

TEST_CASE("A path outside the root folder gives no relative path")
{
    const std::filesystem::path temp = std::filesystem::temp_directory_path();

    CHECK_FALSE(Quasar::MakeRelativePath(temp / "Other" / "cube.obj", temp / "QuasarProject").has_value());
    CHECK_FALSE(Quasar::MakeRelativePath(temp / "QuasarProjectOld" / "a.obj", temp / "QuasarProject").has_value());
}
