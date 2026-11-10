#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/imgui/ImGuiTheme.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <filesystem>

namespace
{
    class ScopedImGuiContext
    {
    public:
        ScopedImGuiContext() { ImGui::CreateContext(); }
        ~ScopedImGuiContext() { ImGui::DestroyContext(); }

        ScopedImGuiContext(const ScopedImGuiContext&) = delete;
        ScopedImGuiContext& operator=(const ScopedImGuiContext&) = delete;
        ScopedImGuiContext(ScopedImGuiContext&&) = delete;
        ScopedImGuiContext& operator=(ScopedImGuiContext&&) = delete;
    };

    bool LoadsOnlyTheEmbeddedFont(const std::filesystem::path& file)
    {
        const ScopedImGuiContext context;
        ImFontAtlas& atlas = *ImGui::GetIO().Fonts;
        return !Quasar::ImGuiTheme::LoadFont(atlas, file) && atlas.Fonts.Size == 1;
    }
}

TEST_CASE("The engine font is copied next to the executable")
{
    const TestHelpers::ScopedLog log;

    const std::filesystem::path font = Quasar::ImGuiTheme::GetFontPath();

    CHECK(std::filesystem::is_regular_file(font));
}

TEST_CASE("The engine font is loaded as the default ImGui font")
{
    const TestHelpers::ScopedLog log;
    const ScopedImGuiContext context;
    ImFontAtlas& atlas = *ImGui::GetIO().Fonts;

    CHECK(Quasar::ImGuiTheme::LoadFont(atlas, Quasar::ImGuiTheme::GetFontPath()));

    REQUIRE(atlas.Fonts.Size == 1);
    CHECK(atlas.Fonts[0]->LegacySize == Quasar::ImGuiTheme::FontSize);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A missing font is replaced by the font embedded in ImGui")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder;

    CHECK(LoadsOnlyTheEmbeddedFont(folder.GetPath() / "missing.ttf"));
    CHECK(LoadsOnlyTheEmbeddedFont({}));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A file that is not a font is replaced by the font embedded in ImGui")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFile text("not_a_font.ttf", "This is not a font");
    const TestHelpers::TemporaryFile tooShort("too_short.ttf", "OT");

    CHECK(LoadsOnlyTheEmbeddedFont(text.GetPath()));
    CHECK(LoadsOnlyTheEmbeddedFont(tooShort.GetPath()));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The theme scales the spacing and the text with the DPI scale")
{
    ImGuiStyle standard;
    ImGuiStyle highDpi;

    Quasar::ImGuiTheme::ApplyStyle(standard, 1.0f);
    Quasar::ImGuiTheme::ApplyStyle(highDpi, 2.0f);

    CHECK(standard.FontSizeBase == Quasar::ImGuiTheme::FontSize);
    CHECK(highDpi.FontSizeBase == Quasar::ImGuiTheme::FontSize);
    CHECK(standard.FontScaleDpi == 1.0f);
    CHECK(highDpi.FontScaleDpi == 2.0f);
    CHECK(highDpi.FramePadding.x == standard.FramePadding.x * 2.0f);
    CHECK(highDpi.ItemSpacing.y == standard.ItemSpacing.y * 2.0f);
    CHECK(highDpi.ScrollbarSize == standard.ScrollbarSize * 2.0f);
}

TEST_CASE("A DPI scale of 0 or less is read as 1")
{
    ImGuiStyle standard;
    ImGuiStyle zero;
    ImGuiStyle negative;

    Quasar::ImGuiTheme::ApplyStyle(standard, 1.0f);
    Quasar::ImGuiTheme::ApplyStyle(zero, 0.0f);
    Quasar::ImGuiTheme::ApplyStyle(negative, -2.0f);

    CHECK(zero.FontScaleDpi == 1.0f);
    CHECK(negative.FontScaleDpi == 1.0f);
    CHECK(zero.FramePadding.x == standard.FramePadding.x);
    CHECK(negative.FramePadding.x == standard.FramePadding.x);
}

TEST_CASE("The theme gives windows an opaque background and square corners for the viewports")
{
    ImGuiStyle style;

    Quasar::ImGuiTheme::ApplyStyle(style, 1.0f);

    CHECK(style.Colors[ImGuiCol_WindowBg].w == 1.0f);
    CHECK(style.WindowRounding == 0.0f);
}
