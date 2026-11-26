#include <platform/opengl/OpenGLPipeline.h>

#include <platform/opengl/OpenGLState.h>

#include <glad/gl.h>

namespace Quasar
{
    namespace
    {
        GLenum GetDepthFunction(DepthCompare compare)
        {
            switch (compare)
            {
                case DepthCompare::Less:
                    return GL_LESS;
                case DepthCompare::LessOrEqual:
                    return GL_LEQUAL;
                case DepthCompare::Greater:
                    return GL_GREATER;
                case DepthCompare::GreaterOrEqual:
                    return GL_GEQUAL;
                case DepthCompare::Equal:
                    return GL_EQUAL;
                case DepthCompare::Always:
                    return GL_ALWAYS;
            }
            return GL_LESS;
        }
    }

    OpenGLPipeline::OpenGLPipeline(const PipelineSpecification& specification, OpenGLState& state) : Pipeline(specification), m_State(&state) {}

    Scope<OpenGLPipeline> OpenGLPipeline::Create(const PipelineSpecification& specification, OpenGLState& state)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }
        return Scope<OpenGLPipeline>(new OpenGLPipeline(specification, state));
    }

    void OpenGLPipeline::Bind()
    {
        ++m_State->GetStatistics().PipelineBinds;
        const PipelineSpecification& specification = GetSpecification();
        specification.Shader->Bind();
        m_State->SetEnabled(OpenGLState::Capability::DepthTest, specification.DepthTest);
        m_State->SetDepthWrite(specification.DepthWrite);
        m_State->SetDepthFunction(GetDepthFunction(specification.Compare));
        m_State->SetEnabled(OpenGLState::Capability::CullFace, specification.Culling != CullMode::None);
        m_State->SetCullFace(specification.Culling == CullMode::Front ? GL_FRONT : GL_BACK);
        m_State->SetFrontFace(GL_CCW);
        m_State->SetEnabled(OpenGLState::Capability::Blend, specification.Blending != BlendMode::None);
        if (specification.Blending == BlendMode::Alpha)
        {
            m_State->SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        else if (specification.Blending == BlendMode::Additive)
        {
            m_State->SetBlendFunction(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
        }
        m_State->SetBlendEquation(GL_FUNC_ADD);
    }
}
