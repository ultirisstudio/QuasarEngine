#pragma once

/// \file
/// \brief Draws the entities of a scene with a renderer.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/scene/Scene.h>

namespace Quasar
{
    /// @brief Draws the entities of a scene that have a TransformComponent and a MeshRendererComponent.
    ///
    /// The scene is drawn from any camera, such as the camera of the editor, or from its primary camera, as in the
    /// game. It is lit by its first DirectionalLightComponent, or by a default DirectionalLight if it has none. The
    /// target is not cleared.
    class SceneRenderer
    {
    public:
        /// @brief Creates a scene renderer that draws with a renderer, which must outlive it.
        explicit SceneRenderer(Renderer& renderer) : m_Renderer(&renderer) {}

        /// @brief Draws a scene seen from a camera.
        /// @param scene The scene.
        /// @param camera The camera, whose viewport size should be the size of the target.
        /// @param target The framebuffer to draw into, or nullptr for the window.
        void Render(Scene& scene, const Camera& camera, Framebuffer* target = nullptr);

        /// @brief Draws a scene seen from its primary camera, at the place of its entity in the world.
        ///
        /// Without a primary camera, nothing is drawn and an engine error is logged, once until a primary camera is
        /// found again.
        void Render(Scene& scene, Framebuffer* target = nullptr);

    private:
        Renderer* m_Renderer;
        bool m_ReportedNoCamera = false;
    };
}
