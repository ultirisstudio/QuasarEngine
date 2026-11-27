#include <quasar/scene/SceneRenderer.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/BoundingBox.h>
#include <quasar/renderer/Color.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/Frustum.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/PointLightComponent.h>
#include <quasar/scene/components/RelationshipComponent.h>
#include <quasar/scene/components/SpotLightComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace Quasar
{
    SceneRenderer::SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings)
        : m_Renderer(&renderer), m_Settings(settings), m_PostProcessing(std::move(resources.PostProcessing)),
          m_SceneFramebuffer(std::move(resources.SceneFramebuffer)), m_ShadowShader(std::move(resources.ShadowShader)),
          m_ShadowPipeline(std::move(resources.ShadowPipeline)), m_DepthPipelines(std::move(resources.DepthPipelines)),
          m_ShadowMap(std::move(resources.ShadowMap)), m_DepthPrepass(std::move(resources.DepthPrepass)),
          m_AmbientOcclusion(std::move(resources.AmbientOcclusion))
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
        resources.PostProcessing = PostProcessStack::Create(context);
        resources.ShadowShader = context.CreateShader(*shadow);
        if (resources.ShadowShader != nullptr)
        {
            resources.ShadowPipeline = context.CreatePipeline({.Shader = resources.ShadowShader.get(), .Culling = CullMode::None});
            for (const CullMode culling : {CullMode::None, CullMode::Back, CullMode::Front})
            {
                resources.DepthPipelines.at(static_cast<std::size_t>(culling)) =
                    context.CreatePipeline({.Shader = resources.ShadowShader.get(), .Culling = culling});
            }
        }
        resources.SceneFramebuffer =
            context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba16F, FramebufferFormat::Depth24Stencil8}});
        resources.ShadowMap = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Depth32F}});
        resources.DepthPrepass = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Depth32F}});
        resources.AmbientOcclusion = AmbientOcclusion::Create(context);
        const bool depthPipelines = std::ranges::all_of(resources.DepthPipelines, [](const Scope<Pipeline>& pipeline) { return pipeline != nullptr; });
        if (resources.PostProcessing == nullptr || resources.ShadowPipeline == nullptr || !depthPipelines || resources.SceneFramebuffer == nullptr ||
            resources.ShadowMap == nullptr || resources.DepthPrepass == nullptr || resources.AmbientOcclusion == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: its post-processing, its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<SceneRenderer>(new SceneRenderer(renderer, std::move(resources), settings));
    }

    bool SceneRenderer::GatherLights(Scene& scene)
    {
        bool castShadows = false;
        m_Lights.Directional.reset();
        m_Lights.Shadow = {};
        m_Lights.AmbientOcclusion = {};
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

    void SceneRenderer::GatherDrawList(Scene& scene, const Frustum& view, const std::optional<Frustum>& light)
    {
        m_DrawList.clear();
        m_DrawOrder.clear();
        m_Statistics = {};
        scene.ForEach<TransformComponent, MeshRendererComponent, RelationshipComponent>(
            [this, &view, &light](Entity entity, const TransformComponent& transform, const MeshRendererComponent& renderer,
                                  const RelationshipComponent& relationship)
            {
                if (renderer.Mesh == nullptr || renderer.Material == nullptr)
                {
                    return;
                }
                ++m_Statistics.Objects;
                const glm::mat4 world = relationship.Parent.IsValid() ? entity.GetWorldMatrix() : transform.GetMatrix();
                const std::optional<BoundingBox>& bounds = renderer.Mesh->GetBounds();
                const std::optional<BoundingBox> box = bounds ? std::optional(bounds->Transformed(world)) : std::nullopt;
                const auto canSee = [&box](const Frustum& frustum) { return !box || frustum.Intersects(*box); };
                const bool visible = canSee(view);
                const bool castsShadow = renderer.CastShadows && light && canSee(*light);
                if (!visible && !castsShadow)
                {
                    return;
                }
                DrawPasses passes = DrawPasses::SceneAndShadow;
                if (!castsShadow)
                {
                    passes = DrawPasses::Scene;
                }
                else if (!visible)
                {
                    passes = DrawPasses::Shadow;
                }
                m_DrawOrder.push_back({.Pipeline = renderer.Material->GetSpecification().Pipeline,
                                       .Material = renderer.Material,
                                       .Mesh = renderer.Mesh,
                                       .Passes = passes,
                                       .Item = static_cast<std::uint32_t>(m_DrawList.size())});
                m_DrawList.push_back(world);
            });
    }

    void SceneRenderer::BuildBatches()
    {
        const std::less<> before;
        std::ranges::sort(m_DrawOrder,
                          [&before](const DrawKey& first, const DrawKey& second)
                          {
                              if (first.Pipeline != second.Pipeline)
                              {
                                  return before(first.Pipeline, second.Pipeline);
                              }
                              if (first.Material != second.Material)
                              {
                                  return before(first.Material, second.Material);
                              }
                              if (first.Mesh != second.Mesh)
                              {
                                  return before(first.Mesh, second.Mesh);
                              }
                              return first.Passes < second.Passes;
                          });

        m_SceneBatches.clear();
        m_ShadowBatches.clear();
        for (const DrawKey& key : m_DrawOrder)
        {
            const std::optional<PlacedObject> object = m_Renderer->PlaceObject(m_DrawList[key.Item]);
            if (!object)
            {
                continue;
            }
            if (key.Passes != DrawPasses::Shadow)
            {
                AddToBatches(m_SceneBatches, {.Mesh = key.Mesh, .Material = key.Material, .First = *object, .Count = 1});
                ++m_Statistics.VisibleObjects;
            }
            if (key.Passes != DrawPasses::Scene)
            {
                AddToBatches(m_ShadowBatches, {.Mesh = key.Mesh, .Material = nullptr, .First = *object, .Count = 1});
                ++m_Statistics.ShadowCasters;
            }
        }
    }

    void SceneRenderer::AddToBatches(std::vector<Batch>& batches, const Batch& object)
    {
        if (!batches.empty())
        {
            Batch& last = batches.back();
            if (last.Mesh == object.Mesh && last.Material == object.Material && last.First.Index + last.Count == object.First.Index)
            {
                ++last.Count;
                return;
            }
        }
        batches.push_back(object);
    }

    void SceneRenderer::Render(Scene& scene, const Camera& camera, Framebuffer* target)
    {
        FrameProfiler* profiler = &m_Renderer->GetContext().GetProfiler();
        std::optional<ShadowCamera> shadow;
        {
            const ProfilerSection section(profiler, "Gather and cull");
            if (GatherLights(scene) && m_Settings.Shadows.Enabled && m_Lights.Directional)
            {
                shadow = ComputeShadowCamera(camera, m_Lights.Directional->Direction, m_Settings.Shadows);
            }
            GatherDrawList(scene, Frustum::FromViewProjection(camera.GetViewProjection()),
                           shadow ? std::optional(Frustum::FromViewProjection(shadow->View.GetViewProjection())) : std::nullopt);
        }
        {
            const ProfilerSection section(profiler, "Sort and batch");
            BuildBatches();
        }
        if (shadow)
        {
            m_Lights.Shadow = RenderShadows(*shadow);
        }

        const glm::uvec2 size =
            target != nullptr ? glm::uvec2(target->GetSpecification().Width, target->GetSpecification().Height) : m_Renderer->GetContext().GetWindowSize();
        if (m_Settings.AmbientOcclusion.Enabled)
        {
            m_Lights.AmbientOcclusion = RenderAmbientOcclusion(camera, size);
        }
        const std::uint32_t samples = std::clamp(m_Settings.Samples, 1U, m_Renderer->GetContext().GetMaxSamples());
        if (m_SceneFramebuffer->GetSpecification().Samples != samples)
        {
            FramebufferSpecification specification = m_SceneFramebuffer->GetSpecification();
            specification.Samples = samples;
            if (Scope<Framebuffer> framebuffer = m_Renderer->GetContext().CreateFramebuffer(specification); framebuffer != nullptr)
            {
                m_SceneFramebuffer = std::move(framebuffer);
            }
            else
            {
                m_Settings.Samples = m_SceneFramebuffer->GetSpecification().Samples;
            }
        }
        m_SceneFramebuffer->Resize(size.x, size.y);

        {
            const ProfilerSection section(profiler, "Scene");
            m_SceneFramebuffer->ClearColor(0, glm::vec4(SrgbToLinear(m_Settings.Background), 1.0f));
            m_SceneFramebuffer->ClearDepthStencil();
            m_Renderer->BeginScene(camera, m_SceneFramebuffer.get(), m_Lights);
            for (const Batch& batch : m_SceneBatches)
            {
                m_Renderer->Submit(*batch.Mesh, *batch.Material, batch.First, batch.Count);
            }
            m_Renderer->EndScene();
        }

        const ProfilerSection section(profiler, "Post-processing");
        m_PostProcessing->Apply(*m_SceneFramebuffer, target, camera, m_Settings.Exposure, m_Settings.Tonemap);
    }

    SceneAmbientOcclusion SceneRenderer::RenderAmbientOcclusion(const Camera& camera, glm::uvec2 size)
    {
        FrameProfiler* profiler = &m_Renderer->GetContext().GetProfiler();
        m_DepthPrepass->Resize(size.x, size.y);
        {
            const ProfilerSection section(profiler, "Depth prepass");
            m_DepthPrepass->ClearDepthStencil();
            m_Renderer->BeginScene(camera, m_DepthPrepass.get(), SceneLights{.Directional = std::nullopt});
            for (const Batch& batch : m_SceneBatches)
            {
                const CullMode culling = batch.Material->GetSpecification().Pipeline->GetSpecification().Culling;
                m_Renderer->Submit(*batch.Mesh, *m_DepthPipelines.at(static_cast<std::size_t>(culling)), batch.First, batch.Count);
            }
            m_Renderer->EndScene();
        }

        const ProfilerSection section(profiler, "Ambient occlusion");
        m_AmbientOcclusion->Compute(*m_DepthPrepass, camera, m_Settings.AmbientOcclusion);
        return {.Map = &m_AmbientOcclusion->GetResult(), .DirectLightingStrength = m_Settings.AmbientOcclusion.DirectLightingStrength};
    }

    DirectionalShadow SceneRenderer::RenderShadows(const ShadowCamera& shadow)
    {
        const ProfilerSection section(&m_Renderer->GetContext().GetProfiler(), "Shadows");
        m_ShadowMap->Resize(shadow.Resolution, shadow.Resolution);

        m_ShadowMap->ClearDepthStencil();
        m_Renderer->BeginScene(shadow.View, m_ShadowMap.get(), SceneLights{.Directional = std::nullopt});
        for (const Batch& batch : m_ShadowBatches)
        {
            m_Renderer->Submit(*batch.Mesh, *m_ShadowPipeline, batch.First, batch.Count);
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
