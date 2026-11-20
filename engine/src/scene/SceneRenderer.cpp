#include <quasar/scene/SceneRenderer.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/Color.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/PointLightComponent.h>
#include <quasar/scene/components/SpotLightComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>

namespace Quasar
{
    SceneRenderer::SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings)
        : m_Renderer(&renderer), m_Settings(settings), m_ToneMapper(std::move(resources.ToneMapper)), m_Framebuffer(std::move(resources.SceneFramebuffer)),
          m_ShadowShader(std::move(resources.ShadowShader)), m_ShadowPipeline(std::move(resources.ShadowPipeline)), m_ShadowMap(std::move(resources.ShadowMap))
    {
    }

    Scope<SceneRenderer> SceneRenderer::Create(Renderer& renderer, const SceneRendererSettings& settings)
    {
        GraphicsContext& context = renderer.GetContext();
        const std::optional<ShaderSpecification> shadow = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "ShadowDepth.glsl");
        if (!shadow)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: the shader of its shadows cannot be read");
            return nullptr;
        }

        Resources resources;
        resources.ToneMapper = ToneMapper::Create(context);
        resources.ShadowShader = context.CreateShader(*shadow);
        if (resources.ShadowShader != nullptr)
        {
            resources.ShadowPipeline = context.CreatePipeline({.Shader = resources.ShadowShader.get(), .Culling = CullMode::None});
        }
        resources.SceneFramebuffer =
            context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba16F, FramebufferFormat::Depth24Stencil8}});
        resources.ShadowMap = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Depth32F}});
        if (resources.ToneMapper == nullptr || resources.ShadowPipeline == nullptr || resources.SceneFramebuffer == nullptr || resources.ShadowMap == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: its tone mapper, its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<SceneRenderer>(new SceneRenderer(renderer, std::move(resources), settings));
    }

    bool SceneRenderer::GatherLights(Scene& scene)
    {
        bool castShadows = false;
        m_Lights.Directional.reset();
        m_Lights.Shadow = {};
        m_Lights.Points.clear();
        m_Lights.Spots.clear();
        auto directionals = scene.GetRegistry().view<DirectionalLightComponent>();
        if (directionals.begin() != directionals.end())
        {
            const Entity entity(*directionals.begin(), &scene);
            const DirectionalLightComponent& light = entity.Get<DirectionalLightComponent>();
            castShadows = light.CastShadows;
            m_Lights.Directional = DirectionalLight{
                .Direction = entity.GetWorldTransform().GetForward(), .Color = light.Color, .Intensity = light.Intensity, .Ambient = light.Ambient};
        }

        scene.ForEach<PointLightComponent>(
            [this](Entity entity, const PointLightComponent& light) {
                m_Lights.Points.push_back(
                    {.Position = glm::vec3(entity.GetWorldMatrix()[3]), .Color = light.Color, .Intensity = light.Intensity, .Range = light.Range});
            });
        scene.ForEach<SpotLightComponent>(
            [this](Entity entity, const SpotLightComponent& light)
            {
                const TransformComponent world = entity.GetWorldTransform();
                m_Lights.Spots.push_back({.Position = world.Position,
                                          .Direction = world.GetForward(),
                                          .Color = light.Color,
                                          .Intensity = light.Intensity,
                                          .Range = light.Range,
                                          .InnerAngle = light.InnerAngle,
                                          .OuterAngle = light.OuterAngle});
            });
        return castShadows;
    }

    void SceneRenderer::GatherDrawList(Scene& scene)
    {
        m_DrawList.clear();
        scene.ForEach<TransformComponent, MeshRendererComponent>(
            [this](Entity entity, const TransformComponent& /*transform*/, const MeshRendererComponent& renderer)
            {
                if (renderer.Mesh != nullptr && renderer.Material != nullptr)
                {
                    m_DrawList.push_back(
                        {.Mesh = renderer.Mesh, .Material = renderer.Material, .Transform = entity.GetWorldMatrix(), .CastShadows = renderer.CastShadows});
                }
            });
    }

    void SceneRenderer::Render(Scene& scene, const Camera& camera, Framebuffer* target)
    {
        GatherDrawList(scene);
        if (GatherLights(scene) && m_Settings.Shadows.Enabled && m_Lights.Directional)
        {
            m_Lights.Shadow = RenderShadows(camera, *m_Lights.Directional);
        }

        const glm::uvec2 size =
            target != nullptr ? glm::uvec2(target->GetSpecification().Width, target->GetSpecification().Height) : m_Renderer->GetContext().GetWindowSize();
        const std::uint32_t samples = std::clamp(m_Settings.Samples, 1U, m_Renderer->GetContext().GetMaxSamples());
        if (m_Framebuffer->GetSpecification().Samples != samples)
        {
            FramebufferSpecification specification = m_Framebuffer->GetSpecification();
            specification.Samples = samples;
            if (Scope<Framebuffer> framebuffer = m_Renderer->GetContext().CreateFramebuffer(specification); framebuffer != nullptr)
            {
                m_Framebuffer = std::move(framebuffer);
            }
            else
            {
                m_Settings.Samples = m_Framebuffer->GetSpecification().Samples;
            }
        }
        m_Framebuffer->Resize(size.x, size.y);
        m_Framebuffer->ClearColor(0, glm::vec4(SrgbToLinear(m_Settings.Background), 1.0f));
        m_Framebuffer->ClearDepthStencil();

        m_Renderer->BeginScene(camera, m_Framebuffer.get(), m_Lights);
        for (const DrawItem& item : m_DrawList)
        {
            m_Renderer->Submit(*item.Mesh, *item.Material, item.Transform);
        }
        m_Renderer->EndScene();
        m_ToneMapper->Apply(*m_Framebuffer, target, m_Settings.Exposure, m_Settings.Tonemap);
    }

    DirectionalShadow SceneRenderer::RenderShadows(const Camera& camera, const DirectionalLight& light)
    {
        const ShadowCamera shadow = ComputeShadowCamera(camera, light.Direction, m_Settings.Shadows);
        m_ShadowMap->Resize(shadow.Resolution, shadow.Resolution);

        m_ShadowMap->ClearDepthStencil();
        m_Renderer->BeginScene(shadow.View, m_ShadowMap.get(), SceneLights{.Directional = std::nullopt});
        for (const DrawItem& item : m_DrawList)
        {
            if (item.CastShadows)
            {
                m_Renderer->Submit(*item.Mesh, *m_ShadowPipeline, item.Transform);
            }
        }
        m_Renderer->EndScene();

        m_ShadowViewProjection = shadow.View.GetViewProjection();
        return {.Map = m_ShadowMap.get(),
                .ViewProjection = m_ShadowViewProjection,
                .DepthBias = m_Settings.Shadows.DepthBias,
                .NormalBias = m_Settings.Shadows.NormalBias * shadow.TexelSize};
    }

    void SceneRenderer::Render(Scene& scene, Framebuffer* target)
    {
        const Entity entity = scene.FindPrimaryCamera();
        if (!entity.IsValid())
        {
            if (!m_ReportedNoCamera)
            {
                QUASAR_ENGINE_ERROR("Cannot draw a scene without a primary camera");
                m_ReportedNoCamera = true;
            }
            return;
        }
        m_ReportedNoCamera = false;

        Camera camera = entity.Get<CameraComponent>().Camera;
        const TransformComponent world = entity.GetWorldTransform();
        camera.SetPosition(world.Position);
        camera.SetOrientation(world.Rotation);
        Render(scene, camera, target);
    }
}
