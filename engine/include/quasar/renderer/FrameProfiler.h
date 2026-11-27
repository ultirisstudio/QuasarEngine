#pragma once

/// \file
/// \brief The time that the processor and the graphics card spend on each step of a frame, such as the shadows or the
/// bloom.
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
    /// @brief The times of one section of a frame, on the processor and on the graphics card.
    struct SectionTiming
    {
        std::string Name;             ///< The name given to FrameProfiler::BeginSection().
        std::uint32_t Depth = 0;      ///< 0 for a section of the frame, 1 for a section inside it, and so on.
        double CpuMilliseconds = 0.0; ///< The time spent by the processor between the start and the end of the section, smoothed.
        double GpuMilliseconds = 0.0; ///< The time spent by the graphics card on the commands of the section, smoothed.
    };

    /// @brief Measures the time that the processor and the graphics card spend on each section of a frame.
    ///
    /// The code marks its steps with BeginSection() and EndSection(), or with a ProfilerSection, between BeginFrame() and
    /// EndFrame(). Sections can be inside each other, for example each effect inside the post-processing. The
    /// Application runs the frames, and the SceneRenderer and the PostProcessStack mark their steps.
    ///
    /// The time on the processor is the time between the start and the end of a section, and the time on the graphics
    /// card is the time between the commands sent at its start and at its end. The frame on the processor lasts from
    /// BeginFrame() to EndFrame(), and the frame on the graphics card is the sum of the sections of depth 0, so the time
    /// the graphics card waits for the processor is not counted.
    ///
    /// The times of a frame are ready a few frames later, once the graphics card has finished it, and both times are
    /// given together. The profiler keeps the queries of the last FramesInFlight frames and reads the ones that are
    /// ready, so it never makes the program wait for the graphics card. A frame that is still not ready after
    /// FramesInFlight frames is skipped.
    ///
    /// The profiler is off at first, so it costs nothing until SetEnabled() turns it on. Each GraphicsContext owns a
    /// profiler: see GraphicsContext::GetProfiler().
    class FrameProfiler
    {
    public:
        /// The number of frames whose times can be waited for at the same time.
        static constexpr std::size_t FramesInFlight = 4;

        /// The part of a new time added to the smoothed time of a section at each frame.
        static constexpr double Smoothing = 0.1;

        /// @brief Creates a profiler that is off.
        /// @param createQuery Creates the queries, usually GraphicsContext::CreateTimestampQuery(). A query that
        /// cannot be created makes the profiler skip that frame.
        /// @param cpuClock Gives the time of the processor in nanoseconds, or nothing for a steady clock.
        explicit FrameProfiler(std::function<Scope<TimestampQuery>()> createQuery, std::function<std::uint64_t()> cpuClock = {});

        ~FrameProfiler() = default;

        FrameProfiler(const FrameProfiler&) = delete;
        FrameProfiler& operator=(const FrameProfiler&) = delete;
        FrameProfiler(FrameProfiler&&) = delete;
        FrameProfiler& operator=(FrameProfiler&&) = delete;

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
        const std::vector<SectionTiming>& GetTimings() const { return m_Timings; }

        /// @brief Returns the smoothed time of the processor for the whole last frame read, in milliseconds, or 0 if no
        /// frame was read.
        double GetFrameCpuMilliseconds() const { return m_FrameCpuMilliseconds; }

        /// @brief Returns the smoothed time of the graphics card for the sections of depth 0 of the last frame read, in
        /// milliseconds, or 0 if no frame was read.
        double GetFrameGpuMilliseconds() const { return m_FrameGpuMilliseconds; }

    private:
        struct Section
        {
            std::string Name;
            std::uint32_t Depth = 0;
            Scope<TimestampQuery> Begin;
            Scope<TimestampQuery> End;
            std::uint64_t CpuBegin = 0;
            std::uint64_t CpuEnd = 0;
        };

        struct Frame
        {
            std::uint64_t CpuBegin = 0;
            std::uint64_t CpuEnd = 0;
            std::vector<Section> Sections;
            std::size_t SectionCount = 0;
            bool Pending = false;
            bool Valid = true;
        };

        bool Prepare(Scope<TimestampQuery>& query);
        bool ReadFrame(Frame& frame);
        void ReadReadyFrames();

        std::function<Scope<TimestampQuery>()> m_CreateQuery;
        std::function<std::uint64_t()> m_CpuClock;
        std::array<Frame, FramesInFlight> m_Frames;
        std::size_t m_Current = 0;
        std::vector<std::size_t> m_OpenSections;
        std::vector<SectionTiming> m_Timings;
        double m_FrameCpuMilliseconds = 0.0;
        double m_FrameGpuMilliseconds = 0.0;
        bool m_HasTimings = false;
        bool m_Enabled = false;
        bool m_InFrame = false;
    };

    /// @brief Measures a section from its creation to its destruction, like BeginSection() and EndSection().
    ///
    /// For example `const ProfilerSection section(profiler, "Shadows");` at the start of a block.
    class ProfilerSection
    {
    public:
        /// @brief Starts the section.
        /// @param profiler The profiler, or nullptr to measure nothing.
        /// @param name The name of the section.
        ProfilerSection(FrameProfiler* profiler, std::string_view name);

        /// @brief Ends the section.
        ~ProfilerSection();

        ProfilerSection(const ProfilerSection&) = delete;
        ProfilerSection& operator=(const ProfilerSection&) = delete;
        ProfilerSection(ProfilerSection&&) = delete;
        ProfilerSection& operator=(ProfilerSection&&) = delete;

    private:
        FrameProfiler* m_Profiler;
    };
}
