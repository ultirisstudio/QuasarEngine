#include <quasar/renderer/GpuProfiler.h>

#include <optional>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr double NanosecondsPerMillisecond = 1'000'000.0;

        std::optional<double> GetMilliseconds(TimestampQuery& begin, TimestampQuery& end)
        {
            const std::optional<std::uint64_t> first = begin.GetNanoseconds();
            const std::optional<std::uint64_t> last = end.GetNanoseconds();
            if (!first || !last)
            {
                return std::nullopt;
            }
            return *last > *first ? static_cast<double>(*last - *first) / NanosecondsPerMillisecond : 0.0;
        }

        double Smooth(double previous, double value)
        {
            return previous + ((value - previous) * GpuProfiler::Smoothing);
        }
    }

    GpuProfiler::GpuProfiler(std::function<Scope<TimestampQuery>()> createQuery) : m_CreateQuery(std::move(createQuery)) {}

    void GpuProfiler::SetEnabled(bool enabled)
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
        m_FrameMilliseconds = 0.0;
        m_HasTimings = false;
    }

    void GpuProfiler::BeginFrame()
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
        frame.Valid = Prepare(frame.Begin) && Prepare(frame.End);
        if (frame.Valid)
        {
            frame.Begin->Record();
        }
        m_InFrame = true;
    }

    void GpuProfiler::EndFrame()
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
        frame.Pending = frame.Valid;
        m_InFrame = false;
        m_Current = (m_Current + 1) % FramesInFlight;
        ReadReadyFrames();
    }

    void GpuProfiler::BeginSection(std::string_view name)
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

    void GpuProfiler::EndSection()
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
    }

    bool GpuProfiler::Prepare(Scope<TimestampQuery>& query)
    {
        if (query == nullptr && m_CreateQuery)
        {
            query = m_CreateQuery();
        }
        return query != nullptr;
    }

    bool GpuProfiler::ReadFrame(Frame& frame)
    {
        const std::optional<double> total = GetMilliseconds(*frame.Begin, *frame.End);
        if (!total)
        {
            return false;
        }

        std::vector<GpuTiming> timings;
        timings.reserve(frame.SectionCount);
        for (std::size_t index = 0; index < frame.SectionCount; ++index)
        {
            const Section& section = frame.Sections[index];
            const std::optional<double> time = GetMilliseconds(*section.Begin, *section.End);
            if (!time)
            {
                return false;
            }
            timings.push_back({.Name = section.Name, .Depth = section.Depth, .Milliseconds = *time});
        }

        std::vector<bool> used(m_Timings.size(), false);
        for (GpuTiming& timing : timings)
        {
            for (std::size_t index = 0; index < m_Timings.size(); ++index)
            {
                if (!used[index] && m_Timings[index].Name == timing.Name && m_Timings[index].Depth == timing.Depth)
                {
                    used[index] = true;
                    timing.Milliseconds = Smooth(m_Timings[index].Milliseconds, timing.Milliseconds);
                    break;
                }
            }
        }

        m_Timings = std::move(timings);
        m_FrameMilliseconds = m_HasTimings ? Smooth(m_FrameMilliseconds, *total) : *total;
        m_HasTimings = true;
        return true;
    }

    void GpuProfiler::ReadReadyFrames()
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

    GpuSection::GpuSection(GpuProfiler* profiler, std::string_view name) : m_Profiler(profiler)
    {
        if (m_Profiler != nullptr)
        {
            m_Profiler->BeginSection(name);
        }
    }

    GpuSection::~GpuSection()
    {
        if (m_Profiler != nullptr)
        {
            m_Profiler->EndSection();
        }
    }
}
