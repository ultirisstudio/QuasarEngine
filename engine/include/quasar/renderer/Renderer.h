#pragma once

/// \file
/// \brief The renderer: draws meshes with their materials, seen from a camera and lit by a directional light.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Quasar
{
    /// @brief A light that comes from the same direction everywhere, such as the sun.
    struct DirectionalLight
    {
        /// The direction in which the light goes, which does not need a length of 1. A zero direction goes down.
        glm::vec3 Direction{-0.4f, -1.0f, -0.6f};
        glm::vec3 Color{1.0f};  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f; ///< The factor of the color.
        float Ambient = 0.1f;   ///< The part of the light that reaches every surface, even the ones that face away.
    };

    /// @brief Draws meshes with their materials, seen from a camera and lit by a directional light, into a framebuffer or
    /// the window.
    ///
    /// The shaders read the camera, the object and the light from these uniform blocks, and the materials use the
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
    /// };
    /// layout(std140, binding = 3) uniform Light
    /// {
    ///     vec3 u_LightDirection; // The direction in which the light goes, with a length of 1.
    ///     float u_LightAmbient;
    ///     vec3 u_LightColor;     // The linear color multiplied by the intensity.
    /// };
    /// ```
    /// A scene is drawn between BeginScene() and EndScene(), and each Submit() draws one mesh at once. The target is not
    /// cleared, so that several scenes can be drawn into it.
    ///
    /// The shaders compute and write linear colors, which can go above 1, for example into a framebuffer of format
    /// FramebufferFormat::Rgba16F. The SceneRenderer turns them into the colors of the screen at the end.
    class Renderer
    {
    public:
        static constexpr std::uint32_t CameraBinding = 0; ///< The binding of the `Camera` block.
        static constexpr std::uint32_t ObjectBinding = 2; ///< The binding of the `Object` block.
        static constexpr std::uint32_t LightBinding = 3;  ///< The binding of the `Light` block.

        /// @brief Creates a renderer that draws with a context.
        /// @return The renderer, or nullptr if the context cannot create its uniform buffers. The reason is logged as
        /// an engine error.
        static Scope<Renderer> Create(GraphicsContext& context);

        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /// @brief Starts a scene seen from a camera.
        /// @param camera The camera, whose viewport size should be the size of the target.
        /// @param target The framebuffer to draw into, or nullptr for the window. It must stay alive until EndScene().
        /// @param light The light of the scene.
        ///
        /// A scene started before the previous one ended is refused with an engine error.
        void BeginScene(const Camera& camera, Framebuffer* target = nullptr, const DirectionalLight& light = {});

        /// @brief Draws a mesh with a material.
        /// @param mesh The mesh.
        /// @param material The material, which must not use the bindings of the renderer.
        /// @param transform The matrix that places the mesh in the world.
        ///
        /// A mesh submitted outside of a scene, or with a material on a binding of the renderer, is not drawn and logs
        /// an engine error.
        void Submit(const Mesh& mesh, Material& material, const glm::mat4& transform);

        /// @brief Ends the scene, and makes the next draws go into the window. Ending without a scene is refused with
        /// an engine error.
        void EndScene();

        /// @brief Returns the context that the renderer draws with.
        GraphicsContext& GetContext() const { return *m_Context; }

    private:
        Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object, Scope<UniformBuffer> light);

        GraphicsContext* m_Context;
        Scope<UniformBuffer> m_CameraBuffer;
        Scope<UniformBuffer> m_ObjectBuffer;
        Scope<UniformBuffer> m_LightBuffer;
        Framebuffer* m_Target = nullptr;
        bool m_InScene = false;
        std::vector<const Material*> m_ReportedMaterials;
    };
}
