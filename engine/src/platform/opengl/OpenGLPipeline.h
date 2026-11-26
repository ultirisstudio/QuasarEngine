#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Pipeline.h>

namespace Quasar
{
    class OpenGLDevice;

    class OpenGLPipeline final : public Pipeline
    {
    public:
        static Scope<OpenGLPipeline> Create(const PipelineSpecification& specification, OpenGLDevice& device);

        ~OpenGLPipeline() override = default;

        OpenGLPipeline(const OpenGLPipeline&) = delete;
        OpenGLPipeline& operator=(const OpenGLPipeline&) = delete;
        OpenGLPipeline(OpenGLPipeline&&) = delete;
        OpenGLPipeline& operator=(OpenGLPipeline&&) = delete;

        void Bind() override;

    private:
        OpenGLPipeline(const PipelineSpecification& specification, OpenGLDevice& device);

        OpenGLDevice* m_Device;
    };
}
