#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Lights.h>

namespace Quasar
{
    class LightsBuffer
    {
    public:
        static Scope<LightsBuffer> Create(GraphicsContext& context);

        void Bind(const SceneLights& lights);

    private:
        explicit LightsBuffer(Scope<FrameUniformBuffer> buffer) : m_Buffer(std::move(buffer)) {}

        Scope<FrameUniformBuffer> m_Buffer;
        bool m_ReportedTooManyLights = false;
    };
}
