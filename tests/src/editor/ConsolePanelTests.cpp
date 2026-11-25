#include "helpers/EditorLocale.h"
#include "helpers/LogHelpers.h"

#include <EditorContext.h>
#include <panels/ConsolePanel.h>

#include <quasar/log/Log.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

using Quasar::Log;
using Quasar::Editor::ConsolePanel;
using Quasar::Editor::EditorContext;

namespace
{
    constexpr float ConsoleWidth = 600.0f;
    constexpr float ConsoleHeight = 300.0f;

    constexpr float SearchStep = 2.0f;

    constexpr std::size_t FirstTestRow = 1;

    constexpr std::size_t CopyItem = 0;
    constexpr std::size_t CopyAllItem = 1;

    constexpr const char* ClearButton = "###console.clear";
    constexpr const char* CopyAllButton = "###console.copy_all";
    constexpr const char* DebugButton = "##DebugFilter";
    constexpr const char* WarningButton = "##WarningFilter";
    constexpr const char* ErrorButton = "##ErrorFilter";

    std::string& CopiedText()
    {
        static std::string copied;
        return copied;
    }

    void WriteEveryLevel()
    {
        Log::App().set_level(spdlog::level::trace);
        Log::App().trace("trace message");
        Log::App().debug("debug message");
        Log::App().info("info message");
        Log::App().warn("warning message");
        Log::App().error("error message");
        Log::App().critical("critical message");
    }

    bool WasCopied(std::string_view text)
    {
        return CopiedText().find(text) != std::string::npos;
    }

    bool WereCopiedInOrder(std::initializer_list<std::string_view> texts)
    {
        std::size_t position = 0;
        for (const std::string_view text : texts)
        {
            position = CopiedText().find(text, position);
            if (position == std::string::npos)
            {
                return false;
            }
            position += text.size();
        }
        return true;
    }

    class HeadlessImGui
    {
    public:
        explicit HeadlessImGui(ConsolePanel& panel) : m_Panel(&panel), m_Previous(ImGui::GetCurrentContext()), m_Context(ImGui::CreateContext())
        {
            TestHelpers::UseEditorLocale();
            ImGui::SetCurrentContext(m_Context);
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(ConsoleWidth * 2.0f, ConsoleHeight * 2.0f);
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = nullptr;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

            CopiedText().clear();
            ImGui::GetPlatformIO().Platform_SetClipboardTextFn = [](ImGuiContext* /*context*/, const char* text) { CopiedText() = text; };

            Frame();
        }

        ~HeadlessImGui()
        {
            ImGui::DestroyContext(m_Context);
            ImGui::SetCurrentContext(m_Previous);
        }

        HeadlessImGui(const HeadlessImGui&) = delete;
        HeadlessImGui& operator=(const HeadlessImGui&) = delete;
        HeadlessImGui(HeadlessImGui&&) = delete;
        HeadlessImGui& operator=(HeadlessImGui&&) = delete;

        void Frame() const
        {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(ConsoleWidth, ConsoleHeight));
            m_Panel->Draw();
            ImGui::Render();

            for (ImTextureData* texture : ImGui::GetPlatformIO().Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetTexID(1);
                    texture->SetStatus(ImTextureStatus_OK);
                }
                else if (texture->Status == ImTextureStatus_WantDestroy)
                {
                    texture->SetTexID(ImTextureID_Invalid);
                    texture->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }

        float GetRowsHeight() const
        {
            Frame();
            Frame();
            return GetTable()->InnerWindow->ContentSize.y;
        }

        void ClickRow(std::size_t row) const { ClickAt(GetRowCenter(row), ImGuiMouseButton_Left); }

        void CopyRow(std::size_t row) const { ClickMenuItem(row, CopyItem); }

        void CopyAllFromRow(std::size_t row) const { ClickMenuItem(row, CopyAllItem); }

        void ClickButton(const char* label) const
        {
            const ImVec2 button = FindButton(label);
            REQUIRE(button.x >= 0.0f);
            ClickAt(button, ImGuiMouseButton_Left);
        }

    private:
        static ImGuiWindow* GetConsoleWindow()
        {
            ImGuiWindow* window = ImGui::FindWindowByName("###panel.console");
            REQUIRE(window != nullptr);
            return window;
        }

        static ImGuiTable* GetTable()
        {
            ImGuiTable* table = ImGui::TableFindByID(GetConsoleWindow()->GetID("Messages"));
            REQUIRE(table != nullptr);
            return table;
        }

        void ClickMenuItem(std::size_t row, std::size_t item) const
        {
            const ImVec2 cursor = GetRowCenter(row);
            ClickAt(cursor, ImGuiMouseButton_Right);

            const ImGuiStyle& style = ImGui::GetStyle();
            const float itemHeight = ImGui::GetTextLineHeight() + style.ItemSpacing.y;
            const float y = cursor.y + style.WindowPadding.y + (itemHeight * static_cast<float>(item)) + (ImGui::GetTextLineHeight() / 2.0f);
            ClickAt(ImVec2(cursor.x + style.WindowPadding.x + ImGui::GetFontSize(), y), ImGuiMouseButton_Left);
        }

        void ClickAt(ImVec2 point, ImGuiMouseButton button) const
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMousePosEvent(point.x, point.y);
            Frame();
            io.AddMouseButtonEvent(button, true);
            io.AddMouseButtonEvent(button, false);
            Frame();
            Frame();
        }

        static ImVec2 GetRowCenter(std::size_t row)
        {
            const float rowHeight = ImGui::GetTextLineHeight() + (ImGui::GetStyle().CellPadding.y * 2.0f);
            return {GetConsoleWindow()->Pos.x + (ConsoleWidth / 2.0f), GetTable()->InnerRect.Min.y + (rowHeight * (static_cast<float>(row) + 0.5f))};
        }

        ImVec2 FindButton(const char* label) const
        {
            ImGuiWindow* window = GetConsoleWindow();
            const ImGuiID id = window->GetID(label);
            const float y = window->DC.CursorStartPos.y + (ImGui::GetFrameHeight() / 2.0f);
            const auto steps = static_cast<int>(ConsoleWidth / SearchStep);
            for (int step = 0; step < steps; ++step)
            {
                const float x = window->Pos.x + (static_cast<float>(step) * SearchStep);
                ImGui::GetIO().AddMousePosEvent(x, y);
                Frame();
                if (ImGui::GetCurrentContext()->HoveredId == id)
                {
                    return {x, y};
                }
            }
            return {-1.0f, -1.0f};
        }

        ConsolePanel* m_Panel;
        ImGuiContext* m_Previous;
        ImGuiContext* m_Context;
    };
}

TEST_CASE("Clicking a message shows its whole text")
{
    const TestHelpers::ScopedLog log;
    Log::App().error("first line\nsecond line");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    const float collapsed = imgui.GetRowsHeight();

    imgui.ClickRow(FirstTestRow);

    CHECK(imgui.GetRowsHeight() == doctest::Approx(static_cast<double>(collapsed + ImGui::GetTextLineHeight())));
}

TEST_CASE("Clicking an expanded message shows its first line only again")
{
    const TestHelpers::ScopedLog log;
    Log::App().error("first line\nsecond line");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    const float collapsed = imgui.GetRowsHeight();
    imgui.ClickRow(FirstTestRow);
    REQUIRE(imgui.GetRowsHeight() > collapsed);

    imgui.ClickRow(FirstTestRow);

    CHECK(imgui.GetRowsHeight() == doctest::Approx(static_cast<double>(collapsed)));
}

TEST_CASE("Clicking a message expands that message only")
{
    const TestHelpers::ScopedLog log;
    Log::App().info("first\nline");
    Log::App().info("second\nline");
    Log::App().info("third\nline");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    const float collapsed = imgui.GetRowsHeight();

    imgui.ClickRow(FirstTestRow + 1);

    CHECK(imgui.GetRowsHeight() == doctest::Approx(static_cast<double>(collapsed + ImGui::GetTextLineHeight())));
}

TEST_CASE("Copying a message from its menu copies that message only")
{
    const TestHelpers::ScopedLog log;
    Log::App().info("first message");
    Log::App().warn("second message");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.CopyRow(FirstTestRow + 1);

    CHECK(WasCopied("APP"));
    CHECK(WasCopied("second message"));
    CHECK_FALSE(WasCopied("first message"));
}

TEST_CASE("Copying all the messages from the menu of a message copies every message in order")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.CopyAllFromRow(FirstTestRow);

    CHECK(WereCopiedInOrder({"QuasarEngine", "trace message", "debug message", "info message", "warning message", "error message", "critical message"}));
}

TEST_CASE("Copying all the messages from the menu of a message leaves out the hidden categories")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    imgui.ClickButton(WarningButton);

    imgui.CopyAllFromRow(FirstTestRow);

    CHECK_FALSE(WasCopied("warning message"));
    CHECK(WereCopiedInOrder({"trace message", "debug message", "info message", "error message", "critical message"}));
}

TEST_CASE("A copied message keeps its whole text")
{
    const TestHelpers::ScopedLog log;
    Log::App().warn("first line\nsecond line");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.CopyRow(FirstTestRow);

    CHECK(WasCopied("first line\nsecond line"));
}

TEST_CASE("Copying all the messages copies every message in order")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(CopyAllButton);

    CHECK(WereCopiedInOrder({"QuasarEngine", "trace message", "debug message", "info message", "warning message", "error message", "critical message"}));
}

TEST_CASE("Clicking a category hides its messages")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(WarningButton);
    imgui.ClickButton(CopyAllButton);

    CHECK_FALSE(WasCopied("warning message"));
    CHECK(WereCopiedInOrder({"trace message", "debug message", "info message", "error message", "critical message"}));
}

TEST_CASE("Each category has its own button")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(DebugButton);
    imgui.ClickButton(ErrorButton);
    imgui.ClickButton(CopyAllButton);

    CHECK(WasCopied("warning message"));
    CHECK_FALSE(WasCopied("QuasarEngine"));
    CHECK_FALSE(WasCopied("trace message"));
    CHECK_FALSE(WasCopied("debug message"));
    CHECK_FALSE(WasCopied("info message"));
    CHECK_FALSE(WasCopied("error message"));
    CHECK_FALSE(WasCopied("critical message"));
}

TEST_CASE("Clicking a hidden category shows its messages again")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    imgui.ClickButton(DebugButton);

    imgui.ClickButton(DebugButton);
    imgui.ClickButton(CopyAllButton);

    CHECK(WereCopiedInOrder({"trace message", "debug message", "info message"}));
}

TEST_CASE("Clicking Clear removes the debug and warning messages but keeps the errors")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(ClearButton);
    imgui.ClickButton(CopyAllButton);

    CHECK(WereCopiedInOrder({"error message", "critical message"}));
    CHECK_FALSE(WasCopied("QuasarEngine"));
    CHECK_FALSE(WasCopied("trace message"));
    CHECK_FALSE(WasCopied("debug message"));
    CHECK_FALSE(WasCopied("info message"));
    CHECK_FALSE(WasCopied("warning message"));
}

TEST_CASE("Clearing the console keeps the messages in the log")
{
    const TestHelpers::ScopedLog log;
    WriteEveryLevel();
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);
    const std::size_t logged = Log::GetMemorySink()->GetMessages().size();

    imgui.ClickButton(ClearButton);

    CHECK(Log::GetMemorySink()->GetMessages().size() == logged);
}

TEST_CASE("Messages written after a clear are shown")
{
    const TestHelpers::ScopedLog log;
    Log::App().info("before");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(ClearButton);
    Log::App().info("after");
    Log::App().warn("warning after");
    imgui.ClickButton(CopyAllButton);

    CHECK_FALSE(WasCopied("before"));
    CHECK(WereCopiedInOrder({"after", "warning after"}));
}

TEST_CASE("Messages written after a clear stay shown when the log drops the cleared messages")
{
    const TestHelpers::ScopedLog log;
    Log::App().info("cleared");
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(ClearButton);
    for (std::size_t index = 0; index < Log::MemoryCapacity; ++index)
    {
        Log::App().info("kept {}", index);
    }
    imgui.ClickButton(CopyAllButton);

    const std::string last = fmt::format("kept {}", Log::MemoryCapacity - 1);
    CHECK(WereCopiedInOrder({"kept 0", "kept 1", last}));
}

TEST_CASE("A console without a log shows no message")
{
    EditorContext context;
    ConsolePanel panel(context);
    const HeadlessImGui imgui(panel);

    imgui.ClickButton(ClearButton);
    imgui.ClickButton(CopyAllButton);

    CHECK(CopiedText().empty());
}
