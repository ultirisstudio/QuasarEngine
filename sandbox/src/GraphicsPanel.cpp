#include "GraphicsPanel.h"

#include <quasar/core/Application.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/Bloom.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/scene/SceneRenderer.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <bit>

namespace Quasar::Sandbox
{
    namespace
    {
        void DrawVideoOptions()
        {
            Window& window = Application::Get().GetWindow();
            ImGui::SeparatorText("Video");
            bool fullscreen = window.IsFullscreen();
            if (ImGui::Checkbox("Fullscreen", &fullscreen))
            {
                window.SetFullscreen(fullscreen);
            }
            bool vsync = window.IsVSync();
            if (ImGui::Checkbox("VSync", &vsync))
            {
                window.SetVSync(vsync);
            }
        }

        void DrawAmbientOcclusionOptions(AmbientOcclusionSettings& settings)
        {
            ImGui::Checkbox("Ambient occlusion", &settings.Enabled);
            if (!settings.Enabled)
            {
                return;
            }
            ImGui::Indent();
            constexpr std::array<const char*, 4> samples{"8", "16", "32", "64"};
            int quality = std::clamp(std::countr_zero(settings.Samples) - 3, 0, 3);
            if (ImGui::Combo("AO samples", &quality, samples.data(), static_cast<int>(samples.size())))
            {
                settings.Samples = 8U << static_cast<unsigned>(quality);
            }
            ImGui::Checkbox("AO half resolution", &settings.HalfResolution);
            ImGui::SliderFloat("AO radius", &settings.Radius, 0.1f, 2.0f, "%.2f");
            ImGui::SliderFloat("AO intensity", &settings.Intensity, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("AO on direct light", &settings.DirectLightingStrength, 0.0f, 1.0f, "%.2f");
            ImGui::Unindent();
        }

        void DrawQualityOptions(SceneRendererSettings& settings)
        {
            ImGui::SeparatorText("Quality");
            constexpr std::array<const char*, 4> antialiasing{"Off", "2x", "4x", "8x"};
            int level = std::min(std::countr_zero(settings.Samples), 3);
            if (ImGui::Combo("Antialiasing", &level, antialiasing.data(), static_cast<int>(antialiasing.size())))
            {
                settings.Samples = 1U << static_cast<unsigned>(level);
            }
            ImGui::Checkbox("Shadows", &settings.Shadows.Enabled);
            constexpr std::array<const char*, 4> resolutions{"512", "1024", "2048", "4096"};
            int resolution = std::clamp(std::countr_zero(settings.Shadows.Resolution) - 9, 0, 3);
            if (ImGui::Combo("Shadow quality", &resolution, resolutions.data(), static_cast<int>(resolutions.size())))
            {
                settings.Shadows.Resolution = 512U << static_cast<unsigned>(resolution);
            }
            ImGui::SliderFloat("Shadow distance", &settings.Shadows.Distance, 5.0f, 100.0f, "%.0f");
            if (ImGui::TreeNode("Shadow biases"))
            {
                ImGui::SliderFloat("Depth bias", &settings.Shadows.DepthBias, 0.0f, 0.005f, "%.4f");
                ImGui::SliderFloat("Normal bias", &settings.Shadows.NormalBias, 0.0f, 4.0f, "%.2f");
                ImGui::TreePop();
            }
            DrawAmbientOcclusionOptions(settings.AmbientOcclusion);

            ImGui::SeparatorText("Image");
            ImGui::SliderFloat("Exposure", &settings.Exposure, 0.1f, 4.0f, "%.2f");
            constexpr std::array<const char*, 3> tonemaps{"None", "Reinhard", "ACES"};
            int tonemap = static_cast<int>(settings.Tonemap);
            ImGui::Combo("Tonemapping", &tonemap, tonemaps.data(), static_cast<int>(tonemaps.size()));
            settings.Tonemap = static_cast<Tonemapping>(tonemap);
        }

        void DrawBloomOptions(Bloom& bloom)
        {
            ImGui::Indent();
            BloomSettings settings = bloom.GetSettings();
            ImGui::SliderFloat("Bloom intensity", &settings.Intensity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bloom threshold", &settings.Threshold, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Bloom soft knee", &settings.SoftKnee, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bloom radius", &settings.Radius, 0.0f, 2.0f, "%.2f");
            bloom.SetSettings(settings);
            ImGui::Unindent();
        }

        void DrawEffectOptions(const PostProcessStack& effects)
        {
            ImGui::SeparatorText("Effects");
            if (effects.GetEffects().empty())
            {
                ImGui::BeginDisabled();
                ImGui::TextUnformatted("No effects");
                ImGui::EndDisabled();
            }
            for (const Scope<PostProcessEffect>& effect : effects.GetEffects())
            {
                bool enabled = effect->IsEnabled();
                if (ImGui::Checkbox(effect->GetName().c_str(), &enabled))
                {
                    effect->SetEnabled(enabled);
                }
                if (auto* bloom = dynamic_cast<Bloom*>(effect.get()); bloom != nullptr && enabled)
                {
                    DrawBloomOptions(*bloom);
                }
            }
        }
    }

    void DrawGraphicsPanel()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImGuiCond placement = ImGui::GetFrameCount() < 3 ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowPos({viewport->WorkPos.x + viewport->WorkSize.x - 10.0f, viewport->WorkPos.y + 10.0f}, placement, {1.0f, 0.0f});
        if (ImGui::Begin("Graphics", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            DrawVideoOptions();
            if (SceneRenderer* renderer = Application::Get().GetSceneRenderer(); renderer != nullptr)
            {
                SceneRendererSettings settings = renderer->GetSettings();
                DrawQualityOptions(settings);
                DrawEffectOptions(renderer->GetPostProcessing());
                renderer->SetSettings(settings);
            }
        }
        ImGui::End();
    }
}
