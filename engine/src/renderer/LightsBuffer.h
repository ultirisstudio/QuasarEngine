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

        void Set(const SceneLights& lights);
        void Bind();

    private:
        explicit LightsBuffer(Scope<UniformBuffer> buffer) : m_Buffer(std::move(buffer)) {}

        Scope<UniformBuffer> m_Buffer;
        bool m_ReportedTooManyLights = false;
    };
}
