#include <quasar/io/EngineResources.h>

#include <doctest/doctest.h>

#include <filesystem>

TEST_CASE("The engine resources are copied into resources/engine, next to the executable")
{
    const std::filesystem::path directory = Quasar::GetEngineResourcesDirectory();

    REQUIRE_FALSE(directory.empty());
    CHECK(directory.filename() == "engine");
    CHECK(directory.parent_path().filename() == "resources");
    CHECK(std::filesystem::is_regular_file(directory / "fonts" / "Inter" / "Inter-Regular.ttf"));
}
