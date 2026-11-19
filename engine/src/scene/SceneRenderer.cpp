#include <quasar/scene/SceneRenderer.h>

#include <quasar/log/Log.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <glm/vec3.hpp>

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
            const auto [transform, light] = lights.get(*lights.begin());
            return {
                .Direction = transform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f), .Color = light.Color, .Intensity = light.Intensity, .Ambient = light.Ambient};
        }
    }

    void SceneRenderer::Render(Scene& scene, const Camera& camera, Framebuffer* target)
    {
        m_Renderer->BeginScene(camera, target, FindLight(scene));
        scene.ForEach<TransformComponent, MeshRendererComponent>(
            [this](Entity /*entity*/, const TransformComponent& transform, const MeshRendererComponent& renderer)
            {
                if (renderer.Mesh != nullptr && renderer.Material != nullptr)
                {
                    m_Renderer->Submit(*renderer.Mesh, *renderer.Material, transform.GetMatrix());
                }
            });
        m_Renderer->EndScene();
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
        if (const TransformComponent* transform = entity.TryGet<TransformComponent>(); transform != nullptr)
        {
            camera.SetPosition(transform->Position);
            camera.SetOrientation(transform->Rotation);
        }
        Render(scene, camera, target);
    }
}
