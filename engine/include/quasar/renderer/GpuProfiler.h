#pragma once

/// \file
/// \brief The time that the graphics card spends on each step of a frame, such as the shadows or the bloom.
/// \author Kurogenshi
/// \date 2026-11-24

#include <quasar/core/Base.h>
#include <quasar/renderer/TimestampQuery.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief The time of one section of a frame on the graphics card.
    struct GpuTiming
    {
        std::string Name;          ///< The name given to GpuProfiler::BeginSection().
        std::uint32_t Depth = 0;   ///< 0 for a section of the frame, 1 for a section inside it, and so on.
        double Milliseconds = 0.0; ///< The time, smoothed over the last frames so that it can be read.
    };

    /// @brief Measures the time that the graphics card spends on each section of a frame.
    ///
    /// The code marks its steps with BeginSection() and EndSection(), or with a GpuSection, between BeginFrame() and
    /// EndFrame(). Sections can be inside each other, for example each effect inside the post-processing. The
    /// Application runs the frames, and the SceneRenderer and the PostProcessStack mark their steps.
    ///
    /// The times of a frame are ready a few frames later. The profiler keeps the queries of the last FramesInFlight
    /// frames and reads the ones that are ready, so it never makes the program wait for the graphics card. A frame
    /// that is still not ready after FramesInFlight frames is skipped.
    ///
    /// The profiler is off at first, so it costs nothing until SetEnabled() turns it on. Each GraphicsContext owns a
    /// profiler: see GraphicsContext::GetProfiler().
    class GpuProfiler
    {
    public:
        /// The number of frames whose times can be waited for at the same time.
        static constexpr std::size_t FramesInFlight = 4;

        /// The part of a new time added to the smoothed time of a section at each frame.
        static constexpr double Smoothing = 0.1;

        /// @brief Creates a profiler that is off.
        /// @param createQuery Creates the queries, usually GraphicsContext::CreateTimestampQuery(). A query that
        /// cannot be created makes the profiler skip that frame.
        explicit GpuProfiler(std::function<Scope<TimestampQuery>()> createQuery);

        ~GpuProfiler() = default;

        GpuProfiler(const GpuProfiler&) = delete;
        GpuProfiler& operator=(const GpuProfiler&) = delete;
        GpuProfiler(GpuProfiler&&) = delete;
        GpuProfiler& operator=(GpuProfiler&&) = delete;

        /// @brief Returns true if the profiler measures the frames.
        bool IsEnabled() const { return m_Enabled; }

        /// @brief Turns the measures on or off. Turning them off forgets the times and the frames not read yet.
        void SetEnabled(bool enabled);

        /// @brief Starts a frame. A frame that is not ended yet is ended first.
        void BeginFrame();

        /// @brief Ends the frame, closes its sections still open, and reads the times of the frames that are ready.
        void EndFrame();

        /// @brief Starts a section inside the frame, or inside the section that is open. It is ignored outside a frame.
        /// @param name The name shown with the time, for example "Shadows".
        void BeginSection(std::string_view name);

        /// @brief Ends the last section started. It is ignored if no section is open.
        void EndSection();

        /// @brief Returns the smoothed times of the sections of the last frame read, in the order they started.
        const std::vector<GpuTiming>& GetTimings() const { return m_Timings; }

        /// @brief Returns the smoothed time of the whole last frame read, in milliseconds, or 0 if no frame was read.
        double GetFrameMilliseconds() const { return m_FrameMilliseconds; }

    private:
        struct Section
        {
            std::string Name;
            std::uint32_t Depth = 0;
            Scope<TimestampQuery> Begin;
            Scope<TimestampQuery> End;
        };

        struct Frame
        {
            Scope<TimestampQuery> Begin;
            Scope<TimestampQuery> End;
            std::vector<Section> Sections;
            std::size_t SectionCount = 0;
            bool Pending = false;
            bool Valid = true;
        };

        bool Prepare(Scope<TimestampQuery>& query);
        bool ReadFrame(Frame& frame);
        void ReadReadyFrames();

        std::function<Scope<TimestampQuery>()> m_CreateQuery;
        std::array<Frame, FramesInFlight> m_Frames;
        std::size_t m_Current = 0;
        std::vector<std::size_t> m_OpenSections;
        std::vector<GpuTiming> m_Timings;
        double m_FrameMilliseconds = 0.0;
        bool m_HasTimings = false;
        bool m_Enabled = false;
        bool m_InFrame = false;
    };

    /// @brief Measures a section from its creation to its destruction, like BeginSection() and EndSection().
    ///
    /// For example `const GpuSection section(profiler, "Shadows");` at the start of a block.
    class GpuSection
    {
    public:
        /// @brief Starts the section.
        /// @param profiler The profiler, or nullptr to measure nothing.
        /// @param name The name of the section.
        GpuSection(GpuProfiler* profiler, std::string_view name);

        /// @brief Ends the section.
        ~GpuSection();

        GpuSection(const GpuSection&) = delete;
        GpuSection& operator=(const GpuSection&) = delete;
        GpuSection(GpuSection&&) = delete;
        GpuSection& operator=(GpuSection&&) = delete;

    private:
        GpuProfiler* m_Profiler;
    };
}
