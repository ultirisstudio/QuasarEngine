#include <quasar/renderer/Pipeline.h>

#include <quasar/log/Log.h>

namespace Quasar
{
    Pipeline::Pipeline(const PipelineSpecification& specification) : m_Specification(specification) {}

    bool Pipeline::IsValid(const PipelineSpecification& specification)
    {
        if (specification.Shader == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a pipeline without a shader");
            return false;
        }
        return true;
    }
}
