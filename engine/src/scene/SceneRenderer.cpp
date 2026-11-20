#include <quasar/scene/SceneRenderer.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/Color.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <optional>
#include <utility>

namespace Quasar
{
    namespace
    {
        DirectionalLight FindLight(Scene& scene)
        {
            auto lights = scene.GetRegistry().view<TransformComponent, DirectionalLightComponent>();
            if (lights.begin() == lights.end())
            {
                return {};
            }
            const Entity entity(*lights.begin(), &scene);
            const DirectionalLightComponent& light = entity.Get<DirectionalLightComponent>();
            return {.Direction = entity.GetWorldTransform().Rotation * glm::vec3(0.0f, 0.0f, -1.0f),
                    .Color = light.Color,
                    .Intensity = light.Intensity,
                    .Ambient = light.Ambient};
        }
    }

    SceneRenderer::SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings)
        : m_Renderer(&renderer), m_Settings(settings), m_PresentShader(std::move(resources.PresentShader)),
          m_PresentPipeline(std::move(resources.PresentPipeline)), m_Framebuffer(std::move(resources.SceneFramebuffer)),
          m_VertexArray(std::move(resources.EmptyVertexArray))
    {
    }

    Scope<SceneRenderer> SceneRenderer::Create(Renderer& renderer, const SceneRendererSettings& settings)
    {
        GraphicsContext& context = renderer.GetContext();
        const std::optional<ShaderSpecification> present = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "Present.glsl");
        if (!present)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: the shader of its last pass cannot be read");
            return nullptr;
        }

        Resources resources;
        resources.PresentShader = context.CreateShader(*present);
        if (resources.PresentShader != nullptr)
        {
            resources.PresentPipeline =
                context.CreatePipeline({.Shader = resources.PresentShader.get(), .DepthTest = false, .DepthWrite = false, .Culling = CullMode::None});
        }
        resources.SceneFramebuffer =
            context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba16F, FramebufferFormat::Depth24Stencil8}});
        resources.EmptyVertexArray = context.CreateVertexArray();
        if (resources.PresentPipeline == nullptr || resources.SceneFramebuffer == nullptr || resources.EmptyVertexArray == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a scene renderer: its pipeline or its framebuffer cannot be created");
            return nullptr;
        }
        return Scope<SceneRenderer>(new SceneRenderer(renderer, std::move(resources), settings));
    }

    void SceneRenderer::Render(Scene& scene, const Camera& camera, Framebuffer* target)
    {
        const glm::uvec2 size =
            target != nullptr ? glm::uvec2(target->GetSpecification().Width, target->GetSpecification().Height) : m_Renderer->GetContext().GetWindowSize();
        m_Framebuffer->Resize(size.x, size.y);
        m_Framebuffer->ClearColor(0, glm::vec4(SrgbToLinear(m_Settings.Background), 1.0f));
        m_Framebuffer->ClearDepthStencil();

        m_Renderer->BeginScene(camera, m_Framebuffer.get(), FindLight(scene));
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
