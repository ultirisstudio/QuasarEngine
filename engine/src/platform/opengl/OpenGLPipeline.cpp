#include <platform/opengl/OpenGLPipeline.h>

#include <platform/opengl/OpenGLDevice.h>

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

    OpenGLPipeline::OpenGLPipeline(const PipelineSpecification& specification, OpenGLDevice& device) : Pipeline(specification), m_Device(&device) {}

    Scope<OpenGLPipeline> OpenGLPipeline::Create(const PipelineSpecification& specification, OpenGLDevice& device)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }
        return Scope<OpenGLPipeline>(new OpenGLPipeline(specification, device));
    }

    void OpenGLPipeline::Bind()
    {
        ++m_Device->GetStatistics().PipelineBinds;
        const PipelineSpecification& specification = GetSpecification();
        specification.Shader->Bind();
        m_Device->GetState().SetEnabled(OpenGLState::Capability::DepthTest, specification.DepthTest);
        m_Device->GetState().SetDepthWrite(specification.DepthWrite);
        m_Device->GetState().SetDepthFunction(GetDepthFunction(specification.Compare));
        m_Device->GetState().SetEnabled(OpenGLState::Capability::CullFace, specification.Culling != CullMode::None);
        m_Device->GetState().SetCullFace(specification.Culling == CullMode::Front ? GL_FRONT : GL_BACK);
        m_Device->GetState().SetFrontFace(GL_CCW);
        m_Device->GetState().SetEnabled(OpenGLState::Capability::Blend, specification.Blending != BlendMode::None);
        if (specification.Blending == BlendMode::Alpha)
        {
            m_Device->GetState().SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        else if (specification.Blending == BlendMode::Additive)
        {
            m_Device->GetState().SetBlendFunction(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
        }
        m_Device->GetState().SetBlendEquation(GL_FUNC_ADD);
    }
}
