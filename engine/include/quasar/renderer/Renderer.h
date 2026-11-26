#pragma once

/// \file
/// \brief The renderer: draws meshes with their materials, seen from a camera and lit by the lights of a scene.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Lights.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <optional>

namespace Quasar
{
    class LightsBuffer;

    /// @brief An object placed in the world for the current frame by Renderer::PlaceObject(), which the submissions of
    /// all the scenes of the frame can share.
    struct PlacedObject
    {
        std::uint32_t Part = 0;  ///< The part of the object block that holds the matrices of the object.
        std::uint64_t Frame = 0; ///< The GraphicsContext::GetFrameNumber() of the frame in which it was placed.
    };

    /// @brief Draws meshes with their materials, seen from a camera and lit by the lights of a scene, into a framebuffer
    /// or the window.
    ///
    /// The shaders read the camera, the object and the lights from these uniform blocks, and the materials use the
    /// binding 1:
    /// ```glsl
    /// layout(std140, binding = 0) uniform Camera
    /// {
    ///     mat4 u_ViewProjection;
    ///     mat4 u_View;
    ///     mat4 u_Projection;
    ///     vec4 u_CameraPosition;
    /// };
    /// layout(std140, binding = 2) uniform Object
    /// {
    ///     mat4 u_Model;
    ///     mat3 u_NormalMatrix; // transpose(inverse(mat3(u_Model))), which turns the normals into world space.
    /// };
    /// struct PointLight
    /// {
    ///     vec4 PositionRange; // The position in xyz and the range in w.
    ///     vec4 Color;         // The linear color multiplied by the intensity in rgb.
    /// };
    /// struct SpotLight
    /// {
    ///     vec4 PositionRange;  // The position in xyz and the range in w.
    ///     vec4 DirectionOuter; // The axis, with a length of 1, in xyz and the cosine of the outer angle in w.
    ///     vec4 ColorInner;     // The linear color multiplied by the intensity in rgb and the cosine of the inner angle in w.
    /// };
    /// layout(std140, binding = 3) uniform Lights
    /// {
    ///     vec3 u_DirectionalDirection; // The direction in which the light goes, with a length of 1.
    ///     float u_DirectionalAmbient;
    ///     vec3 u_DirectionalColor;     // The linear color multiplied by the intensity, black without a directional light.
    ///     int u_PointLightCount;
    ///     int u_SpotLightCount;
    ///     mat4 u_ShadowViewProjection; // DirectionalShadow::ViewProjection.
    ///     vec4 u_Shadow;               // 1 with shadows, the depth bias, the normal bias and 1 / the size of the map.
    ///     vec4 u_AmbientOcclusion;     // 1 with ambient occlusion and the share applied to the direct lights.
    ///     PointLight u_PointLights[32];
    ///     SpotLight u_SpotLights[32];
    /// };
    /// ```
    /// The bindings are the ones of ShaderBindings, and the shadow map is read from the texture slot
    /// ShaderBindings::ShadowMap, with `layout(binding = 15) uniform sampler2DShadow u_ShadowMap;`. The ambient
    /// occlusion is read from the slot ShaderBindings::AmbientOcclusionMap, with
    /// `layout(binding = 14) uniform sampler2D u_AmbientOcclusionMap;`, at the pixel being drawn. A scene uses at most
    /// MaxPointLights point lights and MaxSpotLights spot lights. The next ones are not used, and an engine warning is
    /// logged once until the scene has few enough lights again.
    ///
    /// A scene is drawn between BeginScene() and EndScene(), and each Submit() draws one mesh at once. The target is not
    /// cleared, so that several scenes can be drawn into it. BeginScene() binds the blocks of the renderer, and each
    /// Submit() binds a pipeline, or the values and the textures of a material, only when they differ from the previous
    /// draw of the scene. So between BeginScene() and EndScene(), the pipelines and the materials submitted must stay
    /// alive, and nothing else should bind pipelines, uniform blocks or textures.
    ///
    /// The shaders compute and write linear colors, which can go above 1, for example into a framebuffer of format
    /// FramebufferFormat::Rgba16F. The SceneRenderer turns them into the colors of the screen at the end.
    class Renderer
    {
    public:
        /// @brief Creates a renderer that draws with a context.
        /// @return The renderer, or nullptr if the context cannot create its uniform buffers. The reason is logged as
        /// an engine error.
        static Scope<Renderer> Create(GraphicsContext& context);

        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /// @brief Starts a scene seen from a camera.
        /// @param camera The camera, whose viewport size should be the size of the target.
        /// @param target The framebuffer to draw into, or nullptr for the window. It must stay alive until EndScene().
        /// @param lights The lights of the scene.
        ///
        /// A scene started before the previous one ended is refused with an engine error.
        void BeginScene(const Camera& camera, Framebuffer* target = nullptr, const SceneLights& lights = {});

        /// @brief Computes the object block of a mesh placed in the world, once for all the scenes of the current
        /// frame, for example the shadows, the depth prepass and the scene itself. It can be called outside of a scene.
        /// @param transform The matrix that places the mesh in the world.
        /// @return The placed object, which Submit() accepts until GraphicsContext::EndFrame(), or nullopt if its block
        /// cannot be written. The reason is logged as an engine error.
        std::optional<PlacedObject> PlaceObject(const glm::mat4& transform);

        /// @brief Draws a mesh with a material.
        /// @param mesh The mesh.
        /// @param material The material.
        /// @param object The place of the mesh, given by PlaceObject() in the current frame.
        ///
        /// A mesh submitted outside of a scene, or placed in another frame, is not drawn and logs an engine error.
        void Submit(const Mesh& mesh, Material& material, PlacedObject object);

        /// @brief Places a mesh with PlaceObject() and draws it with a material.
        void Submit(const Mesh& mesh, Material& material, const glm::mat4& transform);

        /// @brief Draws a mesh with a pipeline and no material, for example to draw only the depth of the scene.
        /// @param mesh The mesh.
        /// @param pipeline The pipeline, whose shader reads only the blocks of the renderer.
        /// @param object The place of the mesh, given by PlaceObject() in the current frame.
        ///
        /// A mesh submitted outside of a scene, or placed in another frame, is not drawn and logs an engine error.
        void Submit(const Mesh& mesh, Pipeline& pipeline, PlacedObject object);

        /// @brief Places a mesh with PlaceObject() and draws it with a pipeline and no material.
        void Submit(const Mesh& mesh, Pipeline& pipeline, const glm::mat4& transform);

        /// @brief Ends the scene, and makes the next draws go into the window. Ending without a scene is refused with
        /// an engine error.
        void EndScene();

        /// @brief Returns the context that the renderer draws with.
        GraphicsContext& GetContext() const { return *m_Context; }

    private:
        Renderer(GraphicsContext& context, Scope<FrameUniformBuffer> camera, Scope<FrameUniformBuffer> object, Scope<LightsBuffer> lights);

        bool CanDraw(PlacedObject object) const;
        void BindPipeline(Pipeline& pipeline);
        void Draw(const Mesh& mesh, PlacedObject object);

        GraphicsContext* m_Context;
        Scope<FrameUniformBuffer> m_CameraBuffer;
        Scope<FrameUniformBuffer> m_ObjectBuffer;
        Scope<LightsBuffer> m_LightsBuffer;
        Framebuffer* m_Target = nullptr;
        const Pipeline* m_BoundPipeline = nullptr;
        const Material* m_BoundMaterial = nullptr;
        bool m_InScene = false;
    };
}
