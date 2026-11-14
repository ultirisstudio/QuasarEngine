#include <quasar/imgui/ImGuiTheme.h>
#include <quasar/io/File.h>
#include <quasar/log/Log.h>

#include <io/ExecutableDirectory.h>

#include <spdlog/fmt/std.h>

#ifdef QUASAR_PLATFORM_ANDROID
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <optional>
#include <span>
#include <system_error>
#include <vector>

namespace Quasar::ImGuiTheme
{
    namespace
    {
        constexpr ImVec4 Rgb(std::uint32_t hex, float alpha = 1.0f)
        {
            constexpr float maxChannel = 255.0f;
            return {static_cast<float>((hex >> 16U) & 0xFFU) / maxChannel, static_cast<float>((hex >> 8U) & 0xFFU) / maxChannel,
                    static_cast<float>(hex & 0xFFU) / maxChannel, alpha};
        }

        constexpr ImVec4 Text = Rgb(0xE6E7EA);
        constexpr ImVec4 TextDisabled = Rgb(0x7E8189);
        constexpr ImVec4 WindowBackground = Rgb(0x1E1F22);
        constexpr ImVec4 PopupBackground = Rgb(0x25262A);
        constexpr ImVec4 TitleBackground = Rgb(0x17181A);
        constexpr ImVec4 Border = Rgb(0x34363B);
        constexpr ImVec4 Frame = Rgb(0x2B2D31);
        constexpr ImVec4 FrameHovered = Rgb(0x35373C);
        constexpr ImVec4 FrameActive = Rgb(0x3E4047);
        constexpr ImVec4 Control = Rgb(0x34363B);
        constexpr ImVec4 ControlHovered = Rgb(0x404349);
        constexpr ImVec4 Accent = Rgb(0x3D7EFF);
        constexpr ImVec4 AccentHovered = Rgb(0x5A92FF);
        constexpr ImVec4 AccentActive = Rgb(0x2F6AE0);
        constexpr ImVec4 Scrollbar = Rgb(0x3A3C42);
        constexpr ImVec4 ScrollbarHovered = Rgb(0x484B52);
        constexpr ImVec4 ScrollbarActive = Rgb(0x55585F);

        constexpr ImVec2 WindowPadding{8.0f, 8.0f};
        constexpr ImVec2 FramePadding{6.0f, 4.0f};
        constexpr ImVec2 ItemSpacing{8.0f, 6.0f};
        constexpr ImVec2 ItemInnerSpacing{6.0f, 4.0f};
        constexpr float IndentSpacing = 16.0f;
        constexpr float ScrollbarSize = 14.0f;
        constexpr float GrabMinSize = 10.0f;
        constexpr float BorderSize = 1.0f;
        constexpr float Rounding = 3.0f;
        constexpr float ScrollbarRounding = 6.0f;

        void ApplyColors(ImGuiStyle& style)
        {
            ImGui::StyleColorsDark(&style);

            auto& colors = style.Colors;
            colors[ImGuiCol_Text] = Text;
            colors[ImGuiCol_TextDisabled] = TextDisabled;
            colors[ImGuiCol_TextSelectedBg] = Rgb(0x3D7EFF, 0.35f);
            colors[ImGuiCol_TextLink] = AccentHovered;
            colors[ImGuiCol_InputTextCursor] = Text;

            colors[ImGuiCol_WindowBg] = WindowBackground;
            colors[ImGuiCol_ChildBg] = Rgb(0x1E1F22, 0.0f);
            colors[ImGuiCol_PopupBg] = PopupBackground;
            colors[ImGuiCol_MenuBarBg] = TitleBackground;
            colors[ImGuiCol_ModalWindowDimBg] = Rgb(0x000000, 0.5f);
            colors[ImGuiCol_Border] = Border;
            colors[ImGuiCol_BorderShadow] = Rgb(0x000000, 0.0f);
            colors[ImGuiCol_Separator] = Border;
            colors[ImGuiCol_SeparatorHovered] = AccentHovered;
            colors[ImGuiCol_SeparatorActive] = Accent;

            colors[ImGuiCol_TitleBg] = TitleBackground;
            colors[ImGuiCol_TitleBgActive] = TitleBackground;
            colors[ImGuiCol_TitleBgCollapsed] = TitleBackground;

            colors[ImGuiCol_FrameBg] = Frame;
            colors[ImGuiCol_FrameBgHovered] = FrameHovered;
            colors[ImGuiCol_FrameBgActive] = FrameActive;

            colors[ImGuiCol_Button] = Control;
            colors[ImGuiCol_ButtonHovered] = ControlHovered;
            colors[ImGuiCol_ButtonActive] = AccentActive;
            colors[ImGuiCol_Header] = Control;
            colors[ImGuiCol_HeaderHovered] = ControlHovered;
            colors[ImGuiCol_HeaderActive] = AccentActive;
            colors[ImGuiCol_CheckMark] = Accent;
            colors[ImGuiCol_SliderGrab] = Accent;
            colors[ImGuiCol_SliderGrabActive] = AccentActive;
            colors[ImGuiCol_ResizeGrip] = Rgb(0x3D7EFF, 0.2f);
            colors[ImGuiCol_ResizeGripHovered] = Rgb(0x3D7EFF, 0.6f);
            colors[ImGuiCol_ResizeGripActive] = Rgb(0x3D7EFF, 0.9f);

            colors[ImGuiCol_ScrollbarBg] = Rgb(0x1E1F22, 0.0f);
            colors[ImGuiCol_ScrollbarGrab] = Scrollbar;
            colors[ImGuiCol_ScrollbarGrabHovered] = ScrollbarHovered;
            colors[ImGuiCol_ScrollbarGrabActive] = ScrollbarActive;

            colors[ImGuiCol_Tab] = TitleBackground;
            colors[ImGuiCol_TabHovered] = ControlHovered;
            colors[ImGuiCol_TabSelected] = WindowBackground;
            colors[ImGuiCol_TabSelectedOverline] = Accent;
            colors[ImGuiCol_TabDimmed] = TitleBackground;
            colors[ImGuiCol_TabDimmedSelected] = WindowBackground;
            colors[ImGuiCol_TabDimmedSelectedOverline] = Rgb(0x3D7EFF, 0.0f);

            colors[ImGuiCol_DockingPreview] = Rgb(0x3D7EFF, 0.7f);
            colors[ImGuiCol_DockingEmptyBg] = TitleBackground;
            colors[ImGuiCol_DragDropTarget] = Accent;
            colors[ImGuiCol_NavCursor] = Accent;
        }

        void ApplySpacing(ImGuiStyle& style)
        {
            style.WindowPadding = WindowPadding;
            style.FramePadding = FramePadding;
            style.ItemSpacing = ItemSpacing;
            style.ItemInnerSpacing = ItemInnerSpacing;
            style.IndentSpacing = IndentSpacing;
            style.ScrollbarSize = ScrollbarSize;
            style.GrabMinSize = GrabMinSize;

            style.WindowBorderSize = BorderSize;
            style.ChildBorderSize = BorderSize;
            style.PopupBorderSize = BorderSize;
            style.FrameBorderSize = 0.0f;
            style.TabBorderSize = 0.0f;

            style.WindowRounding = 0.0f;
            style.ChildRounding = Rounding;
            style.FrameRounding = Rounding;
            style.PopupRounding = Rounding;
            style.GrabRounding = Rounding;
            style.TabRounding = Rounding;
            style.ScrollbarRounding = ScrollbarRounding;
        }

        bool HasFontSignature(const std::vector<std::byte>& data)
        {
            constexpr std::array<std::array<char, 4>, 4> signatures = {
                {{'\0', '\1', '\0', '\0'}, {'O', 'T', 'T', 'O'}, {'t', 'r', 'u', 'e'}, {'t', 't', 'c', 'f'}}};

            return data.size() >= 4 && std::ranges::any_of(signatures, [&data](const std::array<char, 4>& signature)
                                                           { return std::memcmp(data.data(), signature.data(), 4) == 0; });
        }

#ifdef QUASAR_PLATFORM_ANDROID
        std::optional<std::vector<std::byte>> ReadFontData(const std::filesystem::path& file)
        {
            std::size_t size = 0;
            void* loaded = SDL_LoadFile(file.generic_string().c_str(), &size);
            if (loaded == nullptr)
            {
                QUASAR_ENGINE_WARN("Cannot find the font '{}' in the APK, so ImGui uses its embedded font", file);
                return std::nullopt;
            }

            const std::span<const std::byte> bytes(static_cast<const std::byte*>(loaded), size);
            std::vector<std::byte> data(bytes.begin(), bytes.end());
            SDL_free(loaded);
            return data;
        }
#else
        std::optional<std::vector<std::byte>> ReadFontData(const std::filesystem::path& file)
        {
            std::error_code error;
            if (file.empty() || !std::filesystem::is_regular_file(file, error))
            {
                QUASAR_ENGINE_WARN("Cannot find the font '{}', so ImGui uses its embedded font", file);
                return std::nullopt;
            }

            std::optional<std::vector<std::byte>> data = ReadBinaryFile(file);
            if (!data)
            {
                QUASAR_ENGINE_WARN("Cannot read the font '{}', so ImGui uses its embedded font", file);
            }
            return data;
        }
#endif

        std::optional<std::vector<std::byte>> ReadFontFile(const std::filesystem::path& file)
        {
            std::optional<std::vector<std::byte>> data = ReadFontData(file);
            if (!data)
            {
                return std::nullopt;
            }

            if (!HasFontSignature(*data) || data->size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            {
                QUASAR_ENGINE_WARN("'{}' is not a TrueType or OpenType font, so ImGui uses its embedded font", file);
                return std::nullopt;
            }

            return data;
        }
    }

    void ApplyStyle(ImGuiStyle& style, float dpiScale)
    {
        const float scale = dpiScale > 0.0f ? dpiScale : 1.0f;

        style = ImGuiStyle();
        ApplyColors(style);
        ApplySpacing(style);
        style.ScaleAllSizes(scale);
        style.FontSizeBase = FontSize;
        style.FontScaleDpi = scale;
    }

    std::filesystem::path GetFontPath()
    {
        std::filesystem::path font = std::filesystem::path("resources") / "fonts" / "Inter" / "Inter-Regular.ttf";
        if constexpr (PlatformDetection::IsAndroid())
        {
            return font;
        }

        const std::optional<std::filesystem::path> directory = GetExecutableDirectory();
        if (!directory)
        {
            return {};
        }
        return *directory / font;
    }

    bool LoadFont(ImFontAtlas& atlas, const std::filesystem::path& file)
    {
        if (const std::optional<std::vector<std::byte>> data = ReadFontFile(file))
        {
            const int dataSize = static_cast<int>(data->size());
            void* fontData = ImGui::MemAlloc(data->size());
            std::memcpy(fontData, data->data(), data->size());

            if (atlas.AddFontFromMemoryTTF(fontData, dataSize, FontSize) != nullptr)
            {
                return true;
            }

            QUASAR_ENGINE_WARN("ImGui cannot load the font '{}', so it uses its embedded font", file);
        }

        ImFontConfig config;
        config.SizePixels = FontSize;
        atlas.AddFontDefaultVector(&config);
        return false;
    }
}
