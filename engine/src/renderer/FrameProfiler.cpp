#include <quasar/renderer/FrameProfiler.h>

#include <chrono>
#include <optional>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr double NanosecondsPerMillisecond = 1'000'000.0;

        double ToMilliseconds(std::uint64_t first, std::uint64_t last)
        {
            return last > first ? static_cast<double>(last - first) / NanosecondsPerMillisecond : 0.0;
        }

        std::optional<double> GetMilliseconds(TimestampQuery& begin, TimestampQuery& end)
        {
            const std::optional<std::uint64_t> first = begin.GetNanoseconds();
            const std::optional<std::uint64_t> last = end.GetNanoseconds();
            if (!first || !last)
            {
                return std::nullopt;
            }
            return ToMilliseconds(*first, *last);
        }

        std::uint64_t ReadSteadyClock()
        {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        }

        double Smooth(double previous, double value)
        {
            return previous + ((value - previous) * FrameProfiler::Smoothing);
        }
    }

    FrameProfiler::FrameProfiler(std::function<Scope<TimestampQuery>()> createQuery, std::function<std::uint64_t()> cpuClock)
        : m_CreateQuery(std::move(createQuery)), m_CpuClock(cpuClock ? std::move(cpuClock) : ReadSteadyClock)
    {
    }

    void FrameProfiler::SetEnabled(bool enabled)
    {
        m_Enabled = enabled;
        if (enabled)
        {
            return;
        }
        m_InFrame = false;
        m_OpenSections.clear();
        for (Frame& frame : m_Frames)
        {
            frame.Pending = false;
        }
        m_Timings.clear();
        m_FrameCpuMilliseconds = 0.0;
        m_FrameGpuMilliseconds = 0.0;
        m_HasTimings = false;
    }

    void FrameProfiler::BeginFrame()
    {
        if (!m_Enabled)
        {
            return;
        }
        if (m_InFrame)
        {
            EndFrame();
        }

        Frame& frame = m_Frames.at(m_Current);
        if (frame.Pending)
        {
            ReadFrame(frame);
            frame.Pending = false;
        }
        frame.SectionCount = 0;
        frame.CpuBegin = m_CpuClock();
        frame.Valid = Prepare(frame.Begin) && Prepare(frame.End);
        if (frame.Valid)
        {
            frame.Begin->Record();
        }
        m_InFrame = true;
    }

    void FrameProfiler::EndFrame()
    {
        if (!m_InFrame)
        {
            return;
        }
        while (!m_OpenSections.empty())
        {
            EndSection();
        }

        Frame& frame = m_Frames.at(m_Current);
        if (frame.Valid)
        {
            frame.End->Record();
        }
        frame.CpuEnd = m_CpuClock();
        frame.Pending = frame.Valid;
        m_InFrame = false;
        m_Current = (m_Current + 1) % FramesInFlight;
        ReadReadyFrames();
    }

    void FrameProfiler::BeginSection(std::string_view name)
    {
        if (!m_InFrame)
        {
            return;
        }

        Frame& frame = m_Frames.at(m_Current);
        if (frame.SectionCount == frame.Sections.size())
        {
            frame.Sections.emplace_back();
        }
        Section& section = frame.Sections[frame.SectionCount];
        section.Name = name;
        section.Depth = static_cast<std::uint32_t>(m_OpenSections.size());
        section.CpuBegin = m_CpuClock();
        if (Prepare(section.Begin) && Prepare(section.End))
        {
            section.Begin->Record();
        }
        else
        {
            frame.Valid = false;
        }
        m_OpenSections.push_back(frame.SectionCount);
        ++frame.SectionCount;
    }

    void FrameProfiler::EndSection()
    {
        if (!m_InFrame || m_OpenSections.empty())
        {
            return;
        }
        Section& section = m_Frames.at(m_Current).Sections[m_OpenSections.back()];
        m_OpenSections.pop_back();
        if (section.End != nullptr)
        {
            section.End->Record();
        }
        section.CpuEnd = m_CpuClock();
    }

    bool FrameProfiler::Prepare(Scope<TimestampQuery>& query)
    {
        if (query == nullptr && m_CreateQuery)
        {
            query = m_CreateQuery();
        }
        return query != nullptr;
    }

    bool FrameProfiler::ReadFrame(Frame& frame)
    {
        const std::optional<double> total = GetMilliseconds(*frame.Begin, *frame.End);
        if (!total)
        {
            return false;
        }

        std::vector<SectionTiming> timings;
        timings.reserve(frame.SectionCount);
        for (std::size_t index = 0; index < frame.SectionCount; ++index)
        {
            const Section& section = frame.Sections[index];
            const std::optional<double> time = GetMilliseconds(*section.Begin, *section.End);
            if (!time)
            {
                return false;
            }
            timings.push_back(
                {.Name = section.Name, .Depth = section.Depth, .CpuMilliseconds = ToMilliseconds(section.CpuBegin, section.CpuEnd), .GpuMilliseconds = *time});
        }

        std::vector<bool> used(m_Timings.size(), false);
        for (SectionTiming& timing : timings)
        {
            for (std::size_t index = 0; index < m_Timings.size(); ++index)
            {
                if (!used[index] && m_Timings[index].Name == timing.Name && m_Timings[index].Depth == timing.Depth)
                {
                    used[index] = true;
                    timing.CpuMilliseconds = Smooth(m_Timings[index].CpuMilliseconds, timing.CpuMilliseconds);
                    timing.GpuMilliseconds = Smooth(m_Timings[index].GpuMilliseconds, timing.GpuMilliseconds);
                    break;
                }
            }
        }

        m_Timings = std::move(timings);
        const double cpuTotal = ToMilliseconds(frame.CpuBegin, frame.CpuEnd);
        m_FrameCpuMilliseconds = m_HasTimings ? Smooth(m_FrameCpuMilliseconds, cpuTotal) : cpuTotal;
        m_FrameGpuMilliseconds = m_HasTimings ? Smooth(m_FrameGpuMilliseconds, *total) : *total;
        m_HasTimings = true;
        return true;
    }

    void FrameProfiler::ReadReadyFrames()
    {
        for (std::size_t offset = 0; offset < FramesInFlight; ++offset)
        {
            Frame& frame = m_Frames.at((m_Current + offset) % FramesInFlight);
            if (!frame.Pending)
            {
                continue;
            }
            if (!ReadFrame(frame))
            {
                return;
            }
            frame.Pending = false;
        }
    }

    ProfilerSection::ProfilerSection(FrameProfiler* profiler, std::string_view name) : m_Profiler(profiler)
    {
        if (m_Profiler != nullptr)
        {
            m_Profiler->BeginSection(name);
        }
    }

    ProfilerSection::~ProfilerSection()
    {
        if (m_Profiler != nullptr)
        {
            m_Profiler->EndSection();
        }
    }
}
