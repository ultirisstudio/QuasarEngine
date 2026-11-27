#pragma once

#include <quasar/time/Timestep.h>

#include <array>
#include <cstddef>

namespace Quasar
{
    class FrameProfiler;
    class GraphicsContext;
    class SceneRenderer;
}

namespace Quasar::Sandbox
{
    class PerformancePanel
    {
    public:
        void Record(Timestep timestep);
        void Draw(GraphicsContext* context, const SceneRenderer* renderer) const;

    private:
        struct FrameTimes
        {
            float Average = 0.0f;
            float Fastest = 0.0f;
            float Slowest = 0.0f;
        };

        static constexpr std::size_t HistorySize = 240;

        FrameTimes ComputeFrameTimes() const;
        void DrawFrameTimes(const FrameTimes& times) const;

        std::array<float, HistorySize> m_History{};
        std::size_t m_Next = 0;
        std::size_t m_Count = 0;
        bool m_Started = false;
    };
}
