#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Pipeline.h>

namespace Quasar
{
    class OpenGLState;

    class OpenGLPipeline final : public Pipeline
    {
    public:
        static Scope<OpenGLPipeline> Create(const PipelineSpecification& specification, OpenGLState& state);

        ~OpenGLPipeline() override = default;

        OpenGLPipeline(const OpenGLPipeline&) = delete;
        OpenGLPipeline& operator=(const OpenGLPipeline&) = delete;
        OpenGLPipeline(OpenGLPipeline&&) = delete;
        OpenGLPipeline& operator=(OpenGLPipeline&&) = delete;

        void Bind() override;

    private:
        OpenGLPipeline(const PipelineSpecification& specification, OpenGLState& state);

        OpenGLState* m_State;
    };
}
