#include <quasar/core/Base.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/TimestampQuery.h>

#include <doctest/doctest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using Quasar::FrameProfiler;
using Quasar::ProfilerSection;
using Quasar::SectionTiming;

namespace
{
    constexpr std::uint64_t Millisecond = 1'000'000;

    // A graphics card that runs the recorded points in order, and has only reached some of them
    struct FakeGpu
    {
        std::uint64_t Now = 0;
        std::uint64_t CpuNow = 0;
        std::uint64_t Recorded = 0;
        std::uint64_t Reached = 0;
        std::uint64_t Missing = 0;
        bool Unlimited = true;
        int Created = 0;
        int Failures = 0;

        void Advance(double milliseconds) { Now += static_cast<std::uint64_t>(milliseconds * static_cast<double>(Millisecond)); }
        void AdvanceCpu(double milliseconds) { CpuNow += static_cast<std::uint64_t>(milliseconds * static_cast<double>(Millisecond)); }
        void Pause() { Unlimited = false; }
        void ReachAll()
        {
            Reached = Recorded;
            Unlimited = true;
        }
    };

    class FakeQuery final : public Quasar::TimestampQuery
    {
    public:
        explicit FakeQuery(FakeGpu& gpu) : m_Gpu(&gpu) {}

        void Record() override
        {
            m_Time = m_Gpu->Now;
            m_Order = ++m_Gpu->Recorded;
            m_Recorded = true;
        }

        std::optional<std::uint64_t> GetNanoseconds() override
        {
            if (!m_Recorded || m_Order == m_Gpu->Missing || (!m_Gpu->Unlimited && m_Order > m_Gpu->Reached))
            {
                return std::nullopt;
            }
            return m_Time;
        }

    private:
        FakeGpu* m_Gpu;
        std::uint64_t m_Time = 0;
        std::uint64_t m_Order = 0;
        bool m_Recorded = false;
    };

    FrameProfiler CreateProfiler(FakeGpu& gpu)
    {
        return FrameProfiler(
            [&gpu]() -> Quasar::Scope<Quasar::TimestampQuery>
            {
                if (gpu.Failures > 0)
                {
                    --gpu.Failures;
                    return nullptr;
                }
                ++gpu.Created;
                return Quasar::CreateScope<FakeQuery>(gpu);
            },
            [&gpu]() { return gpu.CpuNow; });
    }

    // A frame of 1 ms of setup, then Shadows for 2 ms and Post-processing for 3 ms with Bloom for 1 ms inside it
    void RunFrame(FrameProfiler& profiler, FakeGpu& gpu, double shadows = 2.0)
    {
        profiler.BeginFrame();
        gpu.Advance(1.0);
        profiler.BeginSection("Shadows");
        gpu.Advance(shadows);
        profiler.EndSection();
        {
            const ProfilerSection post(&profiler, "Post-processing");
            gpu.Advance(1.0);
            const ProfilerSection bloom(&profiler, "Bloom");
            gpu.Advance(1.0);
        }
        gpu.Advance(1.0);
        profiler.EndFrame();
    }

    // The same frame, recorded by the processor in 0.5 ms for Shadows and 0.25 ms for Bloom, 1.5 ms in all
    void RunFrameOnCpu(FrameProfiler& profiler, FakeGpu& gpu, double shadows = 0.5)
    {
        profiler.BeginFrame();
        gpu.AdvanceCpu(0.25);
        profiler.BeginSection("Shadows");
        gpu.AdvanceCpu(shadows);
        gpu.Advance(2.0);
        profiler.EndSection();
        {
            const ProfilerSection post(&profiler, "Post-processing");
            gpu.AdvanceCpu(0.25);
            const ProfilerSection bloom(&profiler, "Bloom");
            gpu.AdvanceCpu(0.25);
            gpu.Advance(1.0);
        }
        gpu.AdvanceCpu(0.25);
        profiler.EndFrame();
    }

    bool HasCpuTiming(const std::vector<SectionTiming>& timings, const std::string& name, double milliseconds)
    {
        for (const SectionTiming& timing : timings)
        {
            if (timing.Name == name)
            {
                return timing.CpuMilliseconds == doctest::Approx(milliseconds);
            }
        }
        return false;
    }

    bool HasTiming(const std::vector<SectionTiming>& timings, const std::string& name, std::uint32_t depth, double milliseconds)
    {
        for (const SectionTiming& timing : timings)
        {
            if (timing.Name == name && timing.Depth == depth)
            {
                return timing.GpuMilliseconds == doctest::Approx(milliseconds);
            }
        }
        return false;
    }
}

TEST_SUITE("FrameProfiler")
{
    TEST_CASE("A new profiler is off and creates no query")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);

        RunFrame(profiler, gpu);

        CHECK_FALSE(profiler.IsEnabled());
        CHECK(gpu.Created == 0);
        CHECK(profiler.GetTimings().empty());
        CHECK(profiler.GetFrameGpuMilliseconds() == 0.0);
    }

    TEST_CASE("The profiler gives the time of each section in the order they started, and the sum of the sections as the frame")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        RunFrame(profiler, gpu);

        const std::vector<SectionTiming>& timings = profiler.GetTimings();
        REQUIRE(timings.size() == 3);
        CHECK(timings[0].Name == "Shadows");
        CHECK(timings[1].Name == "Post-processing");
        CHECK(timings[2].Name == "Bloom");
        CHECK(HasTiming(timings, "Shadows", 0, 2.0));
        CHECK(HasTiming(timings, "Post-processing", 0, 2.0));
        CHECK(HasTiming(timings, "Bloom", 1, 1.0));
        CHECK(profiler.GetFrameGpuMilliseconds() == doctest::Approx(4.0));
    }

    TEST_CASE("The times are smoothed over the frames")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        RunFrame(profiler, gpu, 2.0);
        RunFrame(profiler, gpu, 12.0);

        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 2.0 + (10.0 * FrameProfiler::Smoothing)));
        CHECK(HasTiming(profiler.GetTimings(), "Bloom", 1, 1.0));
        CHECK(profiler.GetFrameGpuMilliseconds() == doctest::Approx(4.0 + (10.0 * FrameProfiler::Smoothing)));
    }

    TEST_CASE("A section that appears starts at its time, and a section that is gone is removed")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        RunFrame(profiler, gpu);

        profiler.BeginFrame();
        profiler.BeginSection("Bloom");
        gpu.Advance(4.0);
        profiler.EndSection();
        profiler.EndFrame();

        REQUIRE(profiler.GetTimings().size() == 1);
        CHECK(HasTiming(profiler.GetTimings(), "Bloom", 0, 4.0));
    }

    TEST_CASE("The profiler never waits: it reads a frame once the graphics card has finished it")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        gpu.Pause();

        RunFrame(profiler, gpu, 2.0);
        RunFrame(profiler, gpu, 2.0);
        CHECK(profiler.GetTimings().empty());
        CHECK(profiler.GetFrameGpuMilliseconds() == 0.0);

        gpu.ReachAll();
        RunFrame(profiler, gpu, 2.0);

        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 2.0));
    }

    TEST_CASE("A frame that is still not finished after the frames in flight is skipped, and its queries are reused")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        gpu.Pause();

        for (std::size_t frame = 0; frame < FrameProfiler::FramesInFlight; ++frame)
        {
            RunFrame(profiler, gpu);
        }
        const int created = gpu.Created;
        RunFrame(profiler, gpu, 8.0);
        CHECK(gpu.Created == created);
        CHECK(profiler.GetTimings().empty());

        gpu.ReachAll();
        RunFrame(profiler, gpu, 8.0);
        CHECK(gpu.Created == created);
        constexpr double afterFifth = 2.0 + (6.0 * FrameProfiler::Smoothing);
        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, afterFifth + ((8.0 - afterFifth) * FrameProfiler::Smoothing)));
    }

    TEST_CASE("Sections outside a frame are ignored, and the end of a frame closes the sections still open")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        profiler.BeginSection("Outside");
        profiler.EndSection();
        profiler.EndFrame();
        CHECK(gpu.Created == 0);

        profiler.BeginFrame();
        profiler.EndSection();
        profiler.BeginSection("Open");
        gpu.Advance(3.0);
        profiler.EndFrame();
        REQUIRE(profiler.GetTimings().size() == 1);
        CHECK(HasTiming(profiler.GetTimings(), "Open", 0, 3.0));
        CHECK(profiler.GetFrameGpuMilliseconds() == doctest::Approx(3.0));

        profiler.BeginFrame();
        profiler.BeginSection("Next");
        gpu.Advance(1.0);
        profiler.BeginFrame();
        REQUIRE(profiler.GetTimings().size() == 1);
        CHECK(HasTiming(profiler.GetTimings(), "Next", 0, 1.0));
    }

    TEST_CASE("Turning the profiler off forgets the times")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        RunFrame(profiler, gpu);
        REQUIRE_FALSE(profiler.GetTimings().empty());

        profiler.SetEnabled(false);
        CHECK(profiler.GetTimings().empty());
        CHECK(profiler.GetFrameGpuMilliseconds() == 0.0);

        profiler.SetEnabled(true);
        RunFrame(profiler, gpu, 5.0);
        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 5.0));
    }

    TEST_CASE("A frame finished while the profiler waited for it is read before its queries are reused")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        gpu.Pause();
        for (std::size_t frame = 0; frame < FrameProfiler::FramesInFlight; ++frame)
        {
            RunFrame(profiler, gpu, 1.0 + static_cast<double>(frame));
        }

        gpu.ReachAll();
        profiler.BeginFrame();

        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 1.0));
    }

    TEST_CASE("A frame is read only once all of its times are ready")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        gpu.Missing = 2;
        RunFrame(profiler, gpu, 3.0);
        CHECK(profiler.GetTimings().empty());

        gpu.Missing = 0;
        RunFrame(profiler, gpu, 3.0);
        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 3.0));
    }

    TEST_CASE("A frame whose queries cannot be created is skipped")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        gpu.Failures = 1;
        RunFrame(profiler, gpu);
        CHECK(profiler.GetTimings().empty());

        RunFrame(profiler, gpu, 4.0);
        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 4.0));
        for (std::size_t frame = 0; frame < FrameProfiler::FramesInFlight; ++frame)
        {
            RunFrame(profiler, gpu, 4.0);
        }

        gpu.Failures = 1;
        profiler.BeginFrame();
        for (const char* name : {"Shadows", "Post-processing", "Bloom", "New"})
        {
            profiler.BeginSection(name);
            profiler.EndSection();
        }
        profiler.EndFrame();
        CHECK(profiler.GetTimings().size() == 3);
        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 4.0));
    }

    TEST_CASE("A clock of the graphics card that goes back gives a time of 0 instead of a huge one")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        gpu.Now = 10 * Millisecond;

        profiler.BeginFrame();
        profiler.BeginSection("Shadows");
        gpu.Now = 0;
        profiler.EndSection();
        profiler.EndFrame();

        CHECK(HasTiming(profiler.GetTimings(), "Shadows", 0, 0.0));
        CHECK(profiler.GetFrameGpuMilliseconds() == 0.0);
    }

    TEST_CASE("The profiler gives the time of the processor next to the time of the graphics card")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        RunFrameOnCpu(profiler, gpu);

        const std::vector<SectionTiming>& timings = profiler.GetTimings();
        REQUIRE(timings.size() == 3);
        CHECK(HasCpuTiming(timings, "Shadows", 0.5));
        CHECK(HasCpuTiming(timings, "Post-processing", 0.5));
        CHECK(HasCpuTiming(timings, "Bloom", 0.25));
        CHECK(HasTiming(timings, "Shadows", 0, 2.0));
        CHECK(HasTiming(timings, "Bloom", 1, 1.0));
        CHECK(profiler.GetFrameCpuMilliseconds() == doctest::Approx(1.5));
        CHECK(profiler.GetFrameGpuMilliseconds() == doctest::Approx(3.0));
    }

    TEST_CASE("The times of the processor are smoothed, and given only once the graphics card has finished the frame")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);

        RunFrameOnCpu(profiler, gpu);
        gpu.Pause();
        RunFrameOnCpu(profiler, gpu, 10.5);

        CHECK(HasCpuTiming(profiler.GetTimings(), "Shadows", 0.5));
        CHECK(profiler.GetFrameCpuMilliseconds() == doctest::Approx(1.5));

        gpu.ReachAll();
        RunFrameOnCpu(profiler, gpu);

        const double second = 0.5 + (10.0 * FrameProfiler::Smoothing);
        CHECK(HasCpuTiming(profiler.GetTimings(), "Shadows", second + ((0.5 - second) * FrameProfiler::Smoothing)));
        CHECK(HasCpuTiming(profiler.GetTimings(), "Bloom", 0.25));
        CHECK(profiler.GetFrameCpuMilliseconds() == doctest::Approx(second + 1.0 + ((0.5 - second) * FrameProfiler::Smoothing)));
    }

    TEST_CASE("Turning the profiler off forgets the times of the processor")
    {
        FakeGpu gpu;
        FrameProfiler profiler = CreateProfiler(gpu);
        profiler.SetEnabled(true);
        RunFrameOnCpu(profiler, gpu);

        profiler.SetEnabled(false);

        CHECK(profiler.GetFrameCpuMilliseconds() == 0.0);
    }

    TEST_CASE("Without a clock, the profiler measures the processor with a steady clock")
    {
        FakeGpu gpu;
        FrameProfiler profiler([&gpu]() -> Quasar::Scope<Quasar::TimestampQuery> { return Quasar::CreateScope<FakeQuery>(gpu); });
        profiler.SetEnabled(true);

        profiler.BeginFrame();
        profiler.BeginSection("Sleep");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        profiler.EndSection();
        profiler.EndFrame();

        REQUIRE(profiler.GetTimings().size() == 1);
        CHECK(profiler.GetTimings()[0].CpuMilliseconds >= 1.5);
        CHECK(profiler.GetFrameCpuMilliseconds() >= profiler.GetTimings()[0].CpuMilliseconds);
    }

    TEST_CASE("A section without a profiler measures nothing")
    {
        const ProfilerSection section(nullptr, "Nothing");
        CHECK(true);
    }
}
