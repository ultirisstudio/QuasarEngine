#pragma once

/// \file
/// \brief Draws the entities of a scene with a renderer, in HDR, then turns the image into the colors of the screen.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/Base.h>
#include <quasar/renderer/AmbientOcclusion.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/Frustum.h>
#include <quasar/renderer/Lights.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Shadows.h>
#include <quasar/scene/Scene.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    /// @brief Settings of the image of a SceneRenderer.
    struct SceneRendererSettings
    {
        glm::vec3 Background{0.1f};                  ///< The color behind the entities, in sRGB as in a color picker.
        float Exposure = 1.0f;                       ///< The factor of the linear colors, applied before the tonemapping.
        Tonemapping Tonemap = Tonemapping::Aces;     ///< How the colors above 1 are brought to the screen.
        ShadowSettings Shadows{};                    ///< The shadows of the directional light.
        AmbientOcclusionSettings AmbientOcclusion{}; ///< The darkening of the ambient light in corners and creases.

        /// Samples per pixel of the scene, which smooth the edges of the meshes (MSAA). 1 turns it off, and a value above
        /// GraphicsContext::GetMaxSamples() is lowered to it. If the framebuffer cannot be created with them, the
        /// previous samples are kept and set back here.
        std::uint32_t Samples = 4;
    };

    /// @brief What a SceneRenderer drew at its last Render().
    struct SceneRendererStatistics
    {
        std::uint32_t Objects = 0;        ///< The entities with a mesh and a material.
        std::uint32_t VisibleObjects = 0; ///< The ones that can be in the view of the camera, drawn into the image.
        std::uint32_t ShadowCasters = 0;  ///< The ones that cast shadows and can be in the view of the light, drawn into the shadow map.

        /// @brief Compares all the counts.
        bool operator==(const SceneRendererStatistics&) const = default;
    };

    /// @brief Draws the entities of a scene that have a TransformComponent and a MeshRendererComponent.
    ///
    /// The scene is drawn from any camera, such as the camera of the editor, or from its primary camera, as in the
    /// game. It is lit by its first DirectionalLightComponent, if it has one, and by all its PointLightComponent and
    /// SpotLightComponent, up to the maximums of the Renderer. The meshes cast shadows from the directional light, see
    /// ShadowSettings.
    ///
    /// With SceneRendererSettings::AmbientOcclusion, the depth of the scene is first drawn alone, with the culling of the
    /// pipeline of each material, and an AmbientOcclusion computes from it how much ambient light reaches each pixel,
    /// which the shaders read from the Renderer.
    ///
    /// The scene is drawn in linear colors into a framebuffer of its own, of format FramebufferFormat::Rgba16F, of the
    /// size of the target and with SceneRendererSettings::Samples, cleared with SceneRendererSettings::Background. Its
    /// PostProcessStack then applies the post-processing effects and the tone mapping, converts the colors to sRGB and
    /// writes them over the whole first color attachment of the target, which must be of format Rgba8 or Rgba16F. Each
    /// view, such as a viewport of the editor, should have its own SceneRenderer, which keeps framebuffers of its size.
    class SceneRenderer
    {
    public:
        /// @brief Creates a scene renderer that draws with a renderer, which must outlive it.
        /// @return The scene renderer, or nullptr if the shaders of its shadows or of its tone mapping cannot be read from
        /// the engine resources, or if its framebuffers or its pipelines cannot be created. The reason is logged as an
        /// engine error.
        static Scope<SceneRenderer> Create(Renderer& renderer, const SceneRendererSettings& settings = {});

        ~SceneRenderer() = default;

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) = delete;

        /// @brief Draws a scene seen from a camera.
        ///
        /// Each step is a section of the profiler of the graphics context: "Shadows", "Depth prepass", "Ambient
        /// occlusion", "Scene" and "Post-processing", which holds a section for each effect. See FrameProfiler.
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

        /// @brief Returns the post-processing effects applied to the image of the scene, to add, remove, enable or
        /// disable them.
        PostProcessStack& GetPostProcessing() const { return *m_PostProcessing; }

        /// @brief Returns the framebuffer where the scene is drawn in linear colors, before the post-processing. A Render()
        /// with other SceneRendererSettings::Samples replaces it, so it should not be kept.
        const Framebuffer& GetSceneFramebuffer() const { return *m_SceneFramebuffer; }

        /// @brief Returns the framebuffer where the depth of the scene is drawn from the directional light.
        const Framebuffer& GetShadowMap() const { return *m_ShadowMap; }

        /// @brief Returns the matrix that projected the world onto the shadow map at the last Render() with shadows. It
        /// moves only by whole pixels of the map, so that the shadows do not shimmer when the camera moves.
        const glm::mat4& GetShadowViewProjection() const { return m_ShadowViewProjection; }

        /// @brief Returns what the last Render() drew. The meshes whose box is out of the view of the camera are not
        /// drawn into the image, and the ones out of the view of the light are not drawn into the shadow map.
        const SceneRendererStatistics& GetStatistics() const { return m_Statistics; }

    private:
        struct Resources
        {
            Scope<PostProcessStack> PostProcessing;
            Scope<Framebuffer> SceneFramebuffer;
            Scope<Shader> ShadowShader;
            Scope<Pipeline> ShadowPipeline;
            std::array<Scope<Pipeline>, 3> DepthPipelines;
            Scope<Framebuffer> ShadowMap;
            Scope<Framebuffer> DepthPrepass;
            Scope<Quasar::AmbientOcclusion> AmbientOcclusion;
        };

        struct DrawItem
        {
            const Quasar::Mesh* Mesh = nullptr;
            Quasar::Material* Material = nullptr;
            glm::mat4 World{1.0f};
        };

        struct Batch
        {
            const Quasar::Mesh* Mesh = nullptr;
            Quasar::Material* Material = nullptr;
            PlacedObject First;
            std::uint32_t Count = 0;
        };

        SceneRenderer(Renderer& renderer, Resources resources, const SceneRendererSettings& settings);

        bool GatherLights(Scene& scene);
        void GatherDrawList(Scene& scene, const Frustum& view, const std::optional<Frustum>& light);
        void BuildBatches(std::vector<std::uint32_t>& order, std::vector<Batch>& batches, bool byMaterial);
        DirectionalShadow RenderShadows(const ShadowCamera& shadow);
        SceneAmbientOcclusion RenderAmbientOcclusion(const Camera& camera, glm::uvec2 size);

        Renderer* m_Renderer;
        SceneRendererSettings m_Settings;
        Scope<PostProcessStack> m_PostProcessing;
        Scope<Framebuffer> m_SceneFramebuffer;
        Scope<Shader> m_ShadowShader;
        Scope<Pipeline> m_ShadowPipeline;
        std::array<Scope<Pipeline>, 3> m_DepthPipelines;
        Scope<Framebuffer> m_ShadowMap;
        Scope<Framebuffer> m_DepthPrepass;
        Scope<AmbientOcclusion> m_AmbientOcclusion;
        SceneLights m_Lights;
        std::vector<DrawItem> m_DrawList;
        std::vector<std::uint32_t> m_SceneOrder;
        std::vector<std::uint32_t> m_ShadowOrder;
        std::vector<Batch> m_SceneBatches;
        std::vector<Batch> m_ShadowBatches;
        glm::mat4 m_ShadowViewProjection{1.0f};
        SceneRendererStatistics m_Statistics;
        bool m_ReportedNoCamera = false;
    };
}
