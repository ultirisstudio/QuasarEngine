#include <quasar/renderer/Bloom.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <glm/vec4.hpp>

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

namespace Quasar
{
    namespace
    {
        bool CreateProgram(GraphicsContext& context, std::string_view file, BlendMode blending, Scope<Shader>& shader, Scope<Pipeline>& pipeline)
        {
            const std::optional<ShaderSpecification> specification = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / file);
            if (!specification)
            {
                return false;
            }
            shader = context.CreateShader(*specification);
            if (shader == nullptr)
            {
                return false;
            }
            pipeline =
                context.CreatePipeline({.Shader = shader.get(), .DepthTest = false, .DepthWrite = false, .Culling = CullMode::None, .Blending = blending});
            return pipeline != nullptr;
        }

        glm::vec2 GetTexel(const Framebuffer& framebuffer)
        {
            const FramebufferSpecification& specification = framebuffer.GetSpecification();
            return {1.0f / static_cast<float>(specification.Width), 1.0f / static_cast<float>(specification.Height)};
        }
    }

    Bloom::Bloom(const BloomSettings& settings, Resources resources)
        : PostProcessEffect("Bloom", PostProcessStage::BeforeToneMapping), m_Settings(settings), m_Resources(std::move(resources))
    {
    }

    Scope<Bloom> Bloom::Create(GraphicsContext& context, const BloomSettings& settings)
    {
        Resources resources;
        resources.Pass = FullscreenPass::Create(context);
        bool created = resources.Pass != nullptr;
        created = CreateProgram(context, "BloomDownsample.glsl", BlendMode::None, resources.Downsample.Shader, resources.Downsample.Pipeline) && created;
        created = CreateProgram(context, "BloomUpsample.glsl", BlendMode::Additive, resources.Upsample.Shader, resources.Upsample.Pipeline) && created;
        created = CreateProgram(context, "BloomComposite.glsl", BlendMode::None, resources.Composite.Shader, resources.Composite.Pipeline) && created;
        for (Scope<Framebuffer>& level : resources.Levels)
        {
            level = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba16F}});
            created = created && level != nullptr;
        }
        if (!created)
        {
            QUASAR_ENGINE_ERROR("Cannot create a bloom: its shaders, its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<Bloom>(new Bloom(settings, std::move(resources)));
    }

    void Bloom::ResizeLevels(glm::uvec2 size)
    {
        m_LevelCount = 0;
        while (m_LevelCount < MaxLevels && (m_LevelCount == 0 || size.x > 1 || size.y > 1))
        {
            size = glm::max(size / 2U, glm::uvec2(1));
            m_Resources.Levels.at(m_LevelCount)->Resize(size.x, size.y);
            ++m_LevelCount;
        }
    }

    void Bloom::Apply(const PostProcessInput& input, Framebuffer* target)
    {
        ResizeLevels(input.Size);
        const auto& levels = m_Resources.Levels;
        const float threshold = std::max(m_Settings.Threshold, 0.0f);
        const float knee = threshold * std::clamp(m_Settings.SoftKnee, 0.0f, 1.0f);
        const float radius = std::max(m_Settings.Radius, 0.0f);

        Shader& downsample = *m_Resources.Downsample.Shader;
        downsample.SetVec4("u_Threshold", {threshold, threshold - knee, 2.0f * knee, 0.25f / (knee + 1e-5f)});
        const Framebuffer* source = input.Source;
        for (std::size_t index = 0; index < m_LevelCount; ++index)
        {
            downsample.SetInt("u_Prefilter", index == 0 ? 1 : 0);
            downsample.SetVec2("u_TargetTexel", GetTexel(*levels.at(index)));
            source->BindColorTexture(0, 0);
            m_Resources.Pass->Draw(*m_Resources.Downsample.Pipeline, levels.at(index).get());
            source = levels.at(index).get();
        }

        Shader& upsample = *m_Resources.Upsample.Shader;
        upsample.SetFloat("u_Radius", radius);
        for (std::size_t index = m_LevelCount - 1; index > 0; --index)
        {
            upsample.SetVec2("u_TargetTexel", GetTexel(*levels.at(index - 1)));
            levels.at(index)->BindColorTexture(0, 0);
            m_Resources.Pass->Draw(*m_Resources.Upsample.Pipeline, levels.at(index - 1).get());
        }

        Shader& composite = *m_Resources.Composite.Shader;
        composite.SetVec2("u_TargetTexel", {1.0f / static_cast<float>(input.Size.x), 1.0f / static_cast<float>(input.Size.y)});
        composite.SetFloat("u_Radius", radius);
        composite.SetFloat("u_Intensity", std::max(m_Settings.Intensity, 0.0f) / static_cast<float>(m_LevelCount));
        input.Source->BindColorTexture(0, 0);
        levels.front()->BindColorTexture(0, 1);
        m_Resources.Pass->Draw(*m_Resources.Composite.Pipeline, target);
    }
}
