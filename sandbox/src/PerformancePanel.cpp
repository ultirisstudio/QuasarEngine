#include "PerformancePanel.h"

#include <quasar/core/Application.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/scene/SceneRenderer.h>

#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <span>
#include <string>
#include <string_view>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr std::string_view SwapSection = "Swap and events";
        constexpr ImVec2 GraphSize{340.0f, 70.0f};
        constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;

        void RightAlignedText(const std::string& text)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text.c_str()).x);
            ImGui::TextUnformatted(text.c_str());
        }

        void TimeRow(const char* name, std::uint32_t depth, double cpuMilliseconds, double gpuMilliseconds)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetStyle().IndentSpacing * static_cast<float>(depth)));
            ImGui::TextUnformatted(name);
            ImGui::TableSetColumnIndex(1);
            RightAlignedText(fmt::format("{:.3f}", cpuMilliseconds));
            ImGui::TableSetColumnIndex(2);
            RightAlignedText(fmt::format("{:.3f}", gpuMilliseconds));
        }

        const char* FindLimit(const FrameProfiler& profiler, float frame, bool vsync)
        {
            double swap = 0.0;
            for (const SectionTiming& timing : profiler.GetTimings())
            {
                if (timing.Depth == 0 && timing.Name == SwapSection)
                {
                    swap = timing.CpuMilliseconds;
                }
            }
            const double processor = profiler.GetFrameCpuMilliseconds() - swap;
            const double graphics = profiler.GetFrameGpuMilliseconds();
            if (vsync && std::max(processor, graphics) < 0.8 * static_cast<double>(frame))
            {
                return "the VSync";
            }
            return graphics > processor ? "the graphics card" : "the processor";
        }

        void DrawSections(FrameProfiler& profiler, float frame)
        {
            bool enabled = profiler.IsEnabled();
            if (ImGui::Checkbox("Measure the sections", &enabled))
            {
                profiler.SetEnabled(enabled);
            }
            if (!enabled || profiler.GetTimings().empty())
            {
                return;
            }

            const bool vsync = Application::Get().GetWindow().IsVSync();
            ImGui::TextUnformatted(fmt::format("Limited by {}", FindLimit(profiler, frame, vsync)).c_str());
            if (!ImGui::BeginTable("Sections", 3, TableFlags | ImGuiTableFlags_BordersInnerV))
            {
                return;
            }
            ImGui::TableSetupColumn("Section (ms)");
            ImGui::TableSetupColumn("Processor");
            ImGui::TableSetupColumn("Graphics card");
            ImGui::TableHeadersRow();
            for (const SectionTiming& timing : profiler.GetTimings())
            {
                TimeRow(timing.Name.c_str(), timing.Depth, timing.CpuMilliseconds, timing.GpuMilliseconds);
            }
            TimeRow("Frame", 0, profiler.GetFrameCpuMilliseconds(), profiler.GetFrameGpuMilliseconds());
            ImGui::EndTable();
        }

        void TextRow(const char* name, const std::string& value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(name);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value.c_str());
        }

        void CountRow(const char* name, std::uint64_t value)
        {
            TextRow(name, fmt::format("{}", value));
        }

        void DrawFrameStatistics(const RenderStatistics& statistics)
        {
            if (!ImGui::CollapsingHeader("Commands") || !ImGui::BeginTable("Commands", 2, TableFlags))
            {
                return;
            }
            CountRow("Draws", statistics.Draws);
            CountRow("Triangles", statistics.Triangles);
            CountRow("Pipeline binds", statistics.PipelineBinds);
            CountRow("Framebuffer binds", statistics.FramebufferBinds);
            CountRow("Texture binds", statistics.TextureBinds);
            CountRow("Uniform buffer binds", statistics.UniformBufferBinds);
            CountRow("Storage buffer binds", statistics.StorageBufferBinds);
            CountRow("State changes", statistics.StateChanges);
            CountRow("Uniform changes", statistics.UniformChanges);
            CountRow("Uploads", statistics.Uploads);
            CountRow("Uploaded bytes", statistics.UploadedBytes);
            ImGui::EndTable();
        }

        void DrawSceneStatistics(const SceneRenderer& renderer)
        {
            if (!ImGui::CollapsingHeader("Scene renderer", ImGuiTreeNodeFlags_DefaultOpen) || !ImGui::BeginTable("SceneRenderer", 2, TableFlags))
            {
                return;
            }
            const SceneRendererStatistics& objects = renderer.GetStatistics();
            CountRow("Objects", objects.Objects);
            CountRow("Visible objects", objects.VisibleObjects);
            CountRow("Shadow casters", objects.ShadowCasters);
            const FramebufferSpecification& image = renderer.GetSceneFramebuffer().GetSpecification();
            const FramebufferSpecification& shadows = renderer.GetShadowMap().GetSpecification();
            TextRow("Image", fmt::format("{}x{}, {} samples", image.Width, image.Height, image.Samples));
            TextRow("Shadow map", fmt::format("{}x{}", shadows.Width, shadows.Height));
            ImGui::EndTable();
        }
    }

    void PerformancePanel::Record(Timestep timestep)
    {
        if (!m_Started)
        {
            m_Started = true;
            return;
        }
        m_History.at(m_Next) = timestep.GetMilliseconds();
        m_Next = (m_Next + 1) % HistorySize;
        m_Count = std::min(m_Count + 1, HistorySize);
    }

    void PerformancePanel::Draw(GraphicsContext* context, const SceneRenderer* renderer) const
    {
        ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const FrameTimes times = ComputeFrameTimes();
            DrawFrameTimes(times);
            if (context != nullptr)
            {
                DrawSections(context->GetProfiler(), times.Average);
            }
            if (renderer != nullptr)
            {
                DrawSceneStatistics(*renderer);
            }
            if (context != nullptr)
            {
                DrawFrameStatistics(context->GetFrameStatistics());
            }
        }
        ImGui::End();
    }

    PerformancePanel::FrameTimes PerformancePanel::ComputeFrameTimes() const
    {
        if (m_Count == 0)
        {
            return {};
        }
        const std::span<const float> history(m_History.data(), m_Count);
        const auto [fastest, slowest] = std::ranges::minmax(history);
        return {.Average = std::accumulate(history.begin(), history.end(), 0.0f) / static_cast<float>(m_Count), .Fastest = fastest, .Slowest = slowest};
    }

    void PerformancePanel::DrawFrameTimes(const FrameTimes& times) const
    {
        const float framesPerSecond = times.Average > 0.0f ? 1000.0f / times.Average : 0.0f;
        ImGui::TextUnformatted(fmt::format("{:.0f} frames per second, {:.2f} ms per frame", framesPerSecond, times.Average).c_str());
        ImGui::TextUnformatted(fmt::format("Fastest {:.2f} ms, slowest {:.2f} ms", times.Fastest, times.Slowest).c_str());
        const std::size_t offset = m_Count == HistorySize ? m_Next : 0;
        ImGui::PlotLines("##FrameTimes", m_History.data(), static_cast<int>(m_Count), static_cast<int>(offset), "Frame time", 0.0f,
                         std::max(times.Slowest * 1.25f, 1.0f), GraphSize);
    }
}
