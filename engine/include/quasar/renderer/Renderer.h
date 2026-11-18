#pragma once

/// \file
/// \brief The renderer: draws meshes with their materials, seen from a camera.
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

#include <cstdint>
#include <vector>

namespace Quasar
{
    /// @brief Draws meshes with their materials, seen from a camera, into a framebuffer or the window.
    ///
    /// The shaders read the camera and the object from these uniform blocks, and the materials use the binding 1:
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
    /// ```
    /// A scene is drawn between BeginScene() and EndScene(), and each Submit() draws one mesh at once. The target is not
    /// cleared, so that several scenes can be drawn into it.
    class Renderer
    {
    public:
        static constexpr std::uint32_t CameraBinding = 0; ///< The binding of the `Camera` block.
        static constexpr std::uint32_t ObjectBinding = 2; ///< The binding of the `Object` block.

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
        ///
        /// A scene started before the previous one ended is refused with an engine error.
        void BeginScene(const Camera& camera, Framebuffer* target = nullptr);

        /// @brief Draws a mesh with a material.
        /// @param mesh The mesh.
        /// @param material The material, which must not use the bindings of the renderer.
        /// @param transform The matrix that places the mesh in the world.
        ///
        /// A mesh submitted outside of a scene, or with a material on the binding of the camera or of the object, is
        /// not drawn and logs an engine error.
        void Submit(const Mesh& mesh, Material& material, const glm::mat4& transform);

        /// @brief Ends the scene, and makes the next draws go into the window. Ending without a scene is refused with
        /// an engine error.
        void EndScene();

    private:
        Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object);

        GraphicsContext* m_Context;
        Scope<UniformBuffer> m_CameraBuffer;
        Scope<UniformBuffer> m_ObjectBuffer;
        Framebuffer* m_Target = nullptr;
        bool m_InScene = false;
        std::vector<const Material*> m_ReportedMaterials;
    };
}
