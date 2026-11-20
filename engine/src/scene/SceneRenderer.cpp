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

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace Quasar
{
    namespace
    {
        SceneLights GatherLights(Scene& scene, bool& castShadows)
        {
            SceneLights lights{.Directional = std::nullopt};
            castShadows = false;
            auto directionals = scene.GetRegistry().view<DirectionalLightComponent>();
            if (directionals.begin() != directionals.end())
            {
                const Entity entity(*directionals.begin(), &scene);
                const DirectionalLightComponent& light = entity.Get<DirectionalLightComponent>();
                castShadows = light.CastShadows;
                lights.Directional = DirectionalLight{
                    .Direction = entity.GetWorldTransform().GetForward(), .Color = light.Color, .Intensity = light.Intensity, .Ambient = light.Ambient};
            }

            scene.ForEach<PointLightComponent>(
                [&lights](Entity entity, const PointLightComponent& light) {
                    lights.Points.push_back(
                        {.Position = glm::vec3(entity.GetWorldMatrix()[3]), .Color = light.Color, .Intensity = light.Intensity, .Range = light.Range});
                });
            scene.ForEach<SpotLightComponent>(
                [&lights](Entity entity, const SpotLightComponent& light)
                {
                    const TransformComponent world = entity.GetWorldTransform();
                    lights.Spots.push_back({.Position = world.Position,
                                            .Direction = world.GetForward(),
                                            .Color = light.Color,
                                            .Intensity = light.Intensity,
                                            .Range = light.Range,
                                            .InnerAngle = light.InnerAngle,
                                            .OuterAngle = light.OuterAngle});
                });
            return lights;
        }
    }

    SceneRenderer::SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings)
        : m_Renderer(&renderer), m_Settings(settings), m_PresentShader(std::move(resources.PresentShader)),
          m_PresentPipeline(std::move(resources.PresentPipeline)), m_Framebuffer(std::move(resources.SceneFramebuffer)),
          m_VertexArray(std::move(resources.EmptyVertexArray)), m_ShadowShader(std::move(resources.ShadowShader)),
          m_ShadowPipeline(std::move(resources.ShadowPipeline)), m_ShadowMap(std::move(resources.ShadowMap))
    {
    }

    Scope<SceneRenderer> SceneRenderer::Create(Renderer& renderer, const SceneRendererSettings& settings)
    {
        GraphicsContext& context = renderer.GetContext();
        const std::optional<ShaderSpecification> present = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "Present.glsl");
        const std::optional<ShaderSpecification> shadow = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "ShadowDepth.glsl");
        if (!present || !shadow)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: the shaders of its shadows and of its last pass cannot be read");
            return nullptr;
        }

        Resources resources;
        resources.PresentShader = context.CreateShader(*present);
        if (resources.PresentShader != nullptr)
        {
            resources.PresentPipeline =
                context.CreatePipeline({.Shader = resources.PresentShader.get(), .DepthTest = false, .DepthWrite = false, .Culling = CullMode::None});
        }
        resources.ShadowShader = context.CreateShader(*shadow);
        if (resources.ShadowShader != nullptr)
        {
            resources.ShadowPipeline = context.CreatePipeline({.Shader = resources.ShadowShader.get(), .Culling = CullMode::None});
        }
        resources.SceneFramebuffer =
            context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba16F, FramebufferFormat::Depth24Stencil8}});
        resources.ShadowMap = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Depth32F}});
        resources.EmptyVertexArray = context.CreateVertexArray();
        if (resources.PresentPipeline == nullptr || resources.ShadowPipeline == nullptr || resources.SceneFramebuffer == nullptr ||
            resources.ShadowMap == nullptr || resources.EmptyVertexArray == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<SceneRenderer>(new SceneRenderer(renderer, std::move(resources), settings));
    }

    void SceneRenderer::Render(Scene& scene, const Camera& camera, Framebuffer* target)
    {
        bool castShadows = false;
        SceneLights lights = GatherLights(scene, castShadows);
        if (m_Settings.Shadows.Enabled && castShadows && lights.Directional)
        {
            lights.Shadow = RenderShadows(scene, camera, *lights.Directional);
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

        m_Renderer->BeginScene(camera, m_Framebuffer.get(), lights);
        scene.ForEach<TransformComponent, MeshRendererComponent>(
            [this](Entity entity, const TransformComponent& /*transform*/, const MeshRendererComponent& renderer)
            {
                if (renderer.Mesh != nullptr && renderer.Material != nullptr)
                {
                    m_Renderer->Submit(*renderer.Mesh, *renderer.Material, entity.GetWorldMatrix());
                }
            });
        m_Renderer->EndScene();
        Present(target);
    }

    DirectionalShadow SceneRenderer::RenderShadows(Scene& scene, const Camera& camera, const DirectionalLight& light)
    {
        const ShadowSettings& settings = m_Settings.Shadows;
        const std::uint32_t resolution = std::max(settings.Resolution, 1U);
        const float distance = std::max(settings.Distance, 0.01f);
        m_ShadowMap->Resize(resolution, resolution);

        // The part of the view that receives shadows is wrapped in a sphere, whose size does not change when the camera
        // turns, so that the shadows do not shimmer
        Camera slice = camera;
        if (slice.GetProjectionType() == ProjectionType::Perspective)
        {
            PerspectiveProjection projection = slice.GetPerspective();
            projection.Far = std::max(std::min(projection.Far, distance), projection.Near * 2.0f);
            slice.SetPerspective(projection);
        }
        else
        {
            OrthographicProjection projection = slice.GetOrthographic();
            projection.Far = std::min(projection.Far, projection.Near + distance);
            slice.SetOrthographic(projection);
        }
        const glm::mat4 toWorld = glm::inverse(slice.GetViewProjection());
        std::array<glm::vec3, 8> corners{};
        for (std::size_t index = 0; index < corners.size(); ++index)
        {
            const glm::vec4 corner =
                toWorld * glm::vec4((index & 1U) != 0 ? 1.0f : -1.0f, (index & 2U) != 0 ? 1.0f : -1.0f, (index & 4U) != 0 ? 1.0f : 0.0f, 1.0f);
            corners.at(index) = glm::vec3(corner) / corner.w;
        }
        glm::vec3 center(0.0f);
        for (const glm::vec3& corner : corners)
        {
            center += corner / static_cast<float>(corners.size());
        }
        float radius = 0.0f;
        for (const glm::vec3& corner : corners)
        {
            radius = std::max(radius, glm::distance(corner, center));
        }
        const float texel = 2.0f * radius / static_cast<float>(resolution);

        const float length = glm::length(light.Direction);
        const glm::vec3 direction = length > 0.0f ? light.Direction / length : glm::vec3(0.0f, -1.0f, 0.0f);
        const glm::vec3 up = std::abs(direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        const float backward = radius + distance;
        Camera view;
        view.SetViewportSize(resolution, resolution);
        view.SetOrthographic({.Height = 2.0f * radius, .Near = 0.0f, .Far = backward + radius});
        view.SetPosition(center - (direction * backward));
        view.LookAt(center, up);

        // The center moves by whole pixels of the map, so that the shadows do not shimmer when the camera moves
        glm::vec3 local = glm::conjugate(view.GetOrientation()) * center;
        local.x = std::floor(local.x / texel) * texel;
        local.y = std::floor(local.y / texel) * texel;
        view.SetPosition((view.GetOrientation() * local) - (direction * backward));

        m_ShadowMap->ClearDepthStencil();
        m_Renderer->BeginScene(view, m_ShadowMap.get(), SceneLights{.Directional = std::nullopt});
        scene.ForEach<TransformComponent, MeshRendererComponent>(
            [this](Entity entity, const TransformComponent& /*transform*/, const MeshRendererComponent& renderer)
            {
                if (renderer.Mesh != nullptr && renderer.Material != nullptr && renderer.CastShadows)
                {
                    m_Renderer->Submit(*renderer.Mesh, *m_ShadowPipeline, entity.GetWorldMatrix());
                }
            });
        m_Renderer->EndScene();

        m_ShadowViewProjection = view.GetViewProjection();
        return {.Map = m_ShadowMap.get(), .ViewProjection = m_ShadowViewProjection, .DepthBias = settings.DepthBias, .NormalBias = settings.NormalBias * texel};
    }

    void SceneRenderer::Present(Framebuffer* target)
    {
        GraphicsContext& context = m_Renderer->GetContext();
        if (target != nullptr)
        {
            target->Bind();
        }
        else
        {
            context.BindWindow();
        }

        m_PresentShader->SetFloat("u_Exposure", m_Settings.Exposure);
        m_PresentShader->SetInt("u_Tonemapping", static_cast<int>(m_Settings.Tonemap));
        m_PresentPipeline->Bind();
        m_Framebuffer->BindColorTexture(0, 0);
        context.Draw(*m_VertexArray, 3);

        if (target != nullptr)
        {
            target->Unbind();
            context.BindWindow();
        }
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
