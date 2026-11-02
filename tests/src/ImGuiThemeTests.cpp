#include "NullWindow.h"

#include <quasar/core/ImGuiTheme.h>
#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/io/File.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <vector>

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

    bool HasWarning()
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::any_of(messages, [](const Quasar::LogMessage& message)
                                   { return message.Level >= Quasar::LogLevel::Warning; });
    }
}

TEST_CASE("The engine font is copied next to the executable")
{
    const TestHelpers::ScopedLog log;

    const std::filesystem::path font = Quasar::ImGuiTheme::GetFontPath();

    CHECK(font.filename() == "Inter-Regular.ttf");
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
    CHECK_FALSE(HasWarning());
}

TEST_CASE("A missing font is replaced by the font embedded in ImGui")
{
    const TestHelpers::ScopedLog log;
    const ScopedImGuiContext context;
    ImFontAtlas& atlas = *ImGui::GetIO().Fonts;
    const std::filesystem::path missing = std::filesystem::temp_directory_path() / "quasar_missing_font.ttf";
    std::filesystem::remove(missing);

    CHECK_FALSE(Quasar::ImGuiTheme::LoadFont(atlas, missing));

    CHECK(atlas.Fonts.Size == 1);
    CHECK(HasWarning());
}

TEST_CASE("A file that is not a font is replaced by the font embedded in ImGui")
{
    const TestHelpers::ScopedLog log;
    const ScopedImGuiContext context;
    ImFontAtlas& atlas = *ImGui::GetIO().Fonts;
    const std::filesystem::path text = std::filesystem::temp_directory_path() / "quasar_not_a_font.ttf";
    REQUIRE(Quasar::WriteTextFile(text, "This is not a font"));

    CHECK_FALSE(Quasar::ImGuiTheme::LoadFont(atlas, text));

    CHECK(atlas.Fonts.Size == 1);
    CHECK(HasWarning());
    std::filesystem::remove(text);
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
