#include "helpers/HeadlessImGui.h"
#include "helpers/LogHelpers.h"
#include "helpers/MathHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/Log.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <string>

using TestHelpers::Color;
using TestHelpers::IsNear;

TEST_CASE("The test helpers compare numbers, vectors and matrices up to a tolerance")
{
    CHECK(IsNear(1.0f, 1.0f + 5e-6f));
    CHECK_FALSE(IsNear(1.0f, 1.0f + 2e-5f));
    CHECK(IsNear(1.0f, 1.05f, 0.1f));

    CHECK(IsNear(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(1.0f, 2.0f, 3.0f + 5e-6f)));
    CHECK_FALSE(IsNear(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(1.0f, 2.0f + 2e-5f, 3.0f)));
    CHECK(IsNear(glm::vec3(0.0f), glm::vec3(0.0f, 0.05f, 0.0f), 0.1f));

    glm::mat4 moved(1.0f);
    CHECK(IsNear(moved, glm::mat4(1.0f)));
    moved[3][1] = 2e-5f;
    CHECK_FALSE(IsNear(moved, glm::mat4(1.0f)));
    CHECK(IsNear(moved, glm::mat4(1.0f), 1e-4f));
    moved[3][1] = 0.0f;
    moved[0][0] = 2.0f;
    CHECK_FALSE(IsNear(moved, glm::mat4(1.0f)));
}

TEST_CASE("The test helpers compare colors channel by channel, by 1 unless told otherwise")
{
    CHECK(IsNear(Color{10, 20, 30, 40}, Color{11, 19, 30, 41}));
    CHECK_FALSE(IsNear(Color{10, 20, 30, 40}, Color{10, 20, 30, 42}));
    CHECK_FALSE(IsNear(Color{12, 20, 30, 40}, Color{10, 20, 30, 40}));
    CHECK(IsNear(Color{13, 20, 30, 40}, Color{10, 20, 30, 40}, 3));
}

TEST_CASE("The test helpers count the errors, or the errors that contain a text")
{
    const TestHelpers::ScopedLog log;
    QUASAR_ENGINE_ERROR("First problem");
    QUASAR_ENGINE_ERROR("Second problem");
    QUASAR_ENGINE_WARN("First warning");

    CHECK(TestHelpers::CountErrors() == 2);
    CHECK(TestHelpers::CountErrors("Second") == 1);
    CHECK(TestHelpers::CountErrors("problem") == 2);
    CHECK(TestHelpers::CountErrors("warning") == 0);
}

TEST_CASE("The test helpers count no errors without a log")
{
    CHECK(TestHelpers::CountErrors() == 0);
}

TEST_CASE("The test helpers read the colors of a framebuffer from its top row, and of the window from its bottom row")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Quasar::Scope<Quasar::Framebuffer> framebuffer = context.CreateFramebuffer({.Width = 2, .Height = 3});
    REQUIRE(framebuffer != nullptr);
    framebuffer->ClearColor(0, {1.0f, 0.0f, 0.0f, 1.0f});
    glEnable(GL_SCISSOR_TEST);
    glScissor(1, 0, 1, 1);
    framebuffer->ClearColor(0, {0.0f, 0.0f, 1.0f, 1.0f});
    context.ClearWindow({0.0f, 0.0f, 0.0f, 1.0f});
    glScissor(0, 0, 1, 1);
    context.ClearWindow({0.0f, 1.0f, 0.0f, 1.0f});
    glDisable(GL_SCISSOR_TEST);

    CHECK(IsNear(TestHelpers::ReadColor(*framebuffer, 1, 2), {0, 0, 255, 255}));
    CHECK(IsNear(TestHelpers::ReadColor(*framebuffer, 1, 0), {255, 0, 0, 255}));
    CHECK(IsNear(TestHelpers::ReadColor(*framebuffer, 0, 2), {255, 0, 0, 255}));
    CHECK(IsNear(TestHelpers::ReadWindowPixel(0, 0), {0, 255, 0, 255}));
    CHECK(IsNear(TestHelpers::ReadWindowPixel(1, 0), {0, 0, 0, 255}));
    GLint bound = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
    CHECK(bound == 0);
}

TEST_CASE("The test helpers read one pixel of an 8-bit texture with any pack alignment, and keep the alignment")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        window->GetGraphicsContext()->CreateFramebuffer({.Width = 2, .Height = 2, .Attachments = {Quasar::FramebufferFormat::Red8}});
    REQUIRE(framebuffer != nullptr);
    framebuffer->ClearColor(0, {0.6f, 0.0f, 0.0f, 1.0f});
    glEnable(GL_SCISSOR_TEST);
    glScissor(1, 0, 1, 1);
    framebuffer->ClearColor(0, {0.2f, 0.0f, 0.0f, 1.0f});
    glDisable(GL_SCISSOR_TEST);
    const auto texture = static_cast<GLuint>(framebuffer->GetImGuiTexture(0));
    glPixelStorei(GL_PACK_ALIGNMENT, 8);

    CHECK(TestHelpers::ReadRed8(texture, 1, 0) == 51);
    CHECK(TestHelpers::ReadRed8(texture, 0, 1) == 153);
    GLint alignment = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    CHECK(alignment == 8);
    CHECK(glGetError() == GL_NO_ERROR);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
}

TEST_CASE("The headless ImGui helper types text and presses keys without a window, then restores the previous context")
{
    ImGuiContext* previous = ImGui::GetCurrentContext();
    std::string text;
    int enterPresses = 0;
    {
        const TestHelpers::HeadlessImGui imgui(
            [&]
            {
                ImGui::Begin("Test");
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetKeyboardFocusHere();
                }
                ImGui::InputText("##Text", &text);
                enterPresses += ImGui::IsKeyPressed(ImGuiKey_Enter, false) ? 1 : 0;
                ImGui::End();
            });
        CHECK(ImGui::GetCurrentContext() != previous);
        CHECK(ImGui::GetIO().Fonts->TexData->Status == ImTextureStatus_OK);

        imgui.Frame();
        imgui.Type("abc");
        imgui.Press(ImGuiKey_Backspace);
        imgui.Press(ImGuiKey_Enter);

        CHECK(text == "ab");
        CHECK(enterPresses == 1);
        CHECK_FALSE(TestHelpers::HeadlessImGui::IsPopupOpen());
    }
    CHECK(ImGui::GetCurrentContext() == previous);
}
