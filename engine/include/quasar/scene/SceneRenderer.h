#pragma once

/// \file
/// \brief Draws the entities of a scene with a renderer, in HDR, then turns the image into the colors of the screen.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/Base.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/scene/Scene.h>

#include <glm/vec3.hpp>

#include <cstdint>

namespace Quasar
{
    /// @brief How the linear colors of a scene, which can go above 1, are brought between 0 and 1 for the screen.
    enum class Tonemapping : std::uint8_t
    {
        None,     ///< The colors above 1 are cut to 1, so bright areas lose their details.
        Reinhard, ///< Each channel c becomes c / (1 + c): soft, but the whites never reach 1.
        Aces      ///< The curve of the film industry standard ACES, as fitted by Krzysztof Narkowicz: contrasted, with soft whites.
    };

    /// @brief Settings of the image of a SceneRenderer.
    struct SceneRendererSettings
    {
        glm::vec3 Background{0.1f};              ///< The color behind the entities, in sRGB as in a color picker.
        float Exposure = 1.0f;                   ///< The factor of the linear colors, applied before the tonemapping.
        Tonemapping Tonemap = Tonemapping::Aces; ///< How the colors above 1 are brought to the screen.
    };

    /// @brief Draws the entities of a scene that have a TransformComponent and a MeshRendererComponent.
    ///
    /// The scene is drawn from any camera, such as the camera of the editor, or from its primary camera, as in the
    /// game. It is lit by its first DirectionalLightComponent, or by a default DirectionalLight if it has none.
    ///
    /// The scene is drawn in linear colors into a framebuffer of its own, of format FramebufferFormat::Rgba16F and of
    /// the size of the target, cleared with SceneRendererSettings::Background. A last pass then applies the exposure
    /// and the tonemapping, converts the colors to sRGB and writes them over the whole first color attachment of the
    /// target, which must be of format Rgba8 or Rgba16F. Each view, such as a viewport of the editor, should have its
    /// own SceneRenderer, which keeps a framebuffer of its size.
    class SceneRenderer
    {
    public:
        /// @brief Creates a scene renderer that draws with a renderer, which must outlive it.
        /// @return The scene renderer, or nullptr if the shader of its last pass cannot be read from the engine
        /// resources, or if its framebuffer or its pipeline cannot be created. The reason is logged as an engine error.
        static Scope<SceneRenderer> Create(Renderer& renderer, const SceneRendererSettings& settings = {});

        ~SceneRenderer() = default;

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) = delete;

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

        /// @brief Returns the settings of the image.
        const SceneRendererSettings& GetSettings() const { return m_Settings; }

        /// @brief Replaces the settings of the image, used from the next Render().
        void SetSettings(const SceneRendererSettings& settings) { m_Settings = settings; }

        /// @brief Returns the framebuffer where the scene is drawn in linear colors, before the last pass.
        const Framebuffer& GetSceneFramebuffer() const { return *m_Framebuffer; }

    private:
        struct Resources
        {
            Scope<Shader> PresentShader;
            Scope<Pipeline> PresentPipeline;
            Scope<Framebuffer> SceneFramebuffer;
            Scope<VertexArray> EmptyVertexArray;
        };

        SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings);

        void Present(Framebuffer* target);

        Renderer* m_Renderer;
        SceneRendererSettings m_Settings;
        Scope<Shader> m_PresentShader;
        Scope<Pipeline> m_PresentPipeline;
        Scope<Framebuffer> m_Framebuffer;
        Scope<VertexArray> m_VertexArray;
        bool m_ReportedNoCamera = false;
    };
}
