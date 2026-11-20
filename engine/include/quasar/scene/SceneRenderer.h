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

#include <glm/mat4x4.hpp>
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

    /// @brief Settings of the shadows of the directional light.
    ///
    /// The shadows are drawn into a square map, seen from the light, that covers the part of the view of the camera up
    /// to Distance. A smaller distance gives sharper shadows. Without bias, a surface can shadow itself in stripes,
    /// called shadow acne; too much bias detaches the shadows from the objects.
    struct ShadowSettings
    {
        bool Enabled = true;             ///< Whether the directional light casts shadows.
        std::uint32_t Resolution = 2048; ///< The width and the height of the map, in pixels.
        float Distance = 40.0f;          ///< The distance from the camera up to which the shadows are drawn.
        float DepthBias = 0.0005f;       ///< The depth, from 0 to 1 across the map, removed from each surface before the test.
        float NormalBias = 1.5f;         ///< How far each surface is moved along its normal before the test, in pixels of the map.
    };

    /// @brief Settings of the image of a SceneRenderer.
    struct SceneRendererSettings
    {
        glm::vec3 Background{0.1f};              ///< The color behind the entities, in sRGB as in a color picker.
        float Exposure = 1.0f;                   ///< The factor of the linear colors, applied before the tonemapping.
        Tonemapping Tonemap = Tonemapping::Aces; ///< How the colors above 1 are brought to the screen.
        ShadowSettings Shadows{};                ///< The shadows of the directional light.

        /// Samples per pixel of the scene, which smooth the edges of the meshes (MSAA). 1 turns it off, and a value above
        /// GraphicsContext::GetMaxSamples() is lowered to it. If the framebuffer cannot be created with them, the
        /// previous samples are kept and set back here.
        std::uint32_t Samples = 4;
    };

    /// @brief Draws the entities of a scene that have a TransformComponent and a MeshRendererComponent.
    ///
    /// The scene is drawn from any camera, such as the camera of the editor, or from its primary camera, as in the
    /// game. It is lit by its first DirectionalLightComponent, if it has one, and by all its PointLightComponent and
    /// SpotLightComponent, up to the maximums of the Renderer. The meshes cast shadows from the directional light, see
    /// ShadowSettings.
    ///
    /// The scene is drawn in linear colors into a framebuffer of its own, of format FramebufferFormat::Rgba16F, of the
    /// size of the target and with SceneRendererSettings::Samples, cleared with SceneRendererSettings::Background. A last pass then applies the exposure
    /// and the tonemapping, converts the colors to sRGB and writes them over the whole first color attachment of the
    /// target, which must be of format Rgba8 or Rgba16F. Each view, such as a viewport of the editor, should have its
    /// own SceneRenderer, which keeps a framebuffer of its size.
    class SceneRenderer
    {
    public:
        /// @brief Creates a scene renderer that draws with a renderer, which must outlive it.
        /// @return The scene renderer, or nullptr if the shaders of its shadows and of its last pass cannot be read from
        /// the engine resources, or if its framebuffers or its pipelines cannot be created. The reason is logged as an
        /// engine error.
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

        /// @brief Returns the framebuffer where the scene is drawn in linear colors, before the last pass. A Render()
        /// with other SceneRendererSettings::Samples replaces it, so it should not be kept.
        const Framebuffer& GetSceneFramebuffer() const { return *m_Framebuffer; }

        /// @brief Returns the framebuffer where the depth of the scene is drawn from the directional light.
        const Framebuffer& GetShadowMap() const { return *m_ShadowMap; }

        /// @brief Returns the matrix that projected the world onto the shadow map at the last Render() with shadows. It
        /// moves only by whole pixels of the map, so that the shadows do not shimmer when the camera moves.
        const glm::mat4& GetShadowViewProjection() const { return m_ShadowViewProjection; }

    private:
        struct Resources
        {
            Scope<Shader> PresentShader;
            Scope<Pipeline> PresentPipeline;
            Scope<Framebuffer> SceneFramebuffer;
            Scope<VertexArray> EmptyVertexArray;
            Scope<Shader> ShadowShader;
            Scope<Pipeline> ShadowPipeline;
            Scope<Framebuffer> ShadowMap;
        };

        SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings);

        DirectionalShadow RenderShadows(Scene& scene, const Camera& camera, const DirectionalLight& light);
        void Present(Framebuffer* target);

        Renderer* m_Renderer;
        SceneRendererSettings m_Settings;
        Scope<Shader> m_PresentShader;
        Scope<Pipeline> m_PresentPipeline;
        Scope<Framebuffer> m_Framebuffer;
        Scope<VertexArray> m_VertexArray;
        Scope<Shader> m_ShadowShader;
        Scope<Pipeline> m_ShadowPipeline;
        Scope<Framebuffer> m_ShadowMap;
        glm::mat4 m_ShadowViewProjection{1.0f};
        bool m_ReportedNoCamera = false;
    };
}
