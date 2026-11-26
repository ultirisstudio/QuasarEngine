#include <platform/opengl/OpenGLPipeline.h>

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

        void SetEnabled(GLenum capability, bool enabled)
        {
            if (enabled)
            {
                glEnable(capability);
            }
            else
            {
                glDisable(capability);
            }
        }
    }

    OpenGLPipeline::OpenGLPipeline(const PipelineSpecification& specification, RenderStatistics& statistics)
        : Pipeline(specification), m_Statistics(&statistics)
    {
    }

    Scope<OpenGLPipeline> OpenGLPipeline::Create(const PipelineSpecification& specification, RenderStatistics& statistics)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }
        return Scope<OpenGLPipeline>(new OpenGLPipeline(specification, statistics));
    }

    void OpenGLPipeline::Bind()
    {
        ++m_Statistics->PipelineBinds;
        const PipelineSpecification& specification = GetSpecification();
        specification.Shader->Bind();

        SetEnabled(GL_DEPTH_TEST, specification.DepthTest);
        glDepthMask(specification.DepthWrite ? GL_TRUE : GL_FALSE);
        glDepthFunc(GetDepthFunction(specification.Compare));

        SetEnabled(GL_CULL_FACE, specification.Culling != CullMode::None);
        glCullFace(specification.Culling == CullMode::Front ? GL_FRONT : GL_BACK);
        glFrontFace(GL_CCW);

        SetEnabled(GL_BLEND, specification.Blending != BlendMode::None);
        if (specification.Blending == BlendMode::Alpha)
        {
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        else if (specification.Blending == BlendMode::Additive)
        {
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
        }
        glBlendEquation(GL_FUNC_ADD);
    }
}
