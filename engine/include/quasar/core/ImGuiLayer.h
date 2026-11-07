#pragma once

/// \file
/// \brief The layer that runs Dear ImGui, so that every layer can draw ImGui windows.
/// \author Kurogenshi
/// \date 2026-11-02

#include <quasar/core/Layer.h>
#include <quasar/events/Event.h>

namespace Quasar
{
    class Window;

    /// @brief Overlay that starts Dear ImGui with its SDL and OpenGL backends, and wraps each frame.
    ///
    /// The application creates it when its window has an OpenGL context, and keeps it above every other layer and
    /// overlay. Each frame, the application calls Begin(), then Layer::OnImGuiRender() on every layer from the bottom
    /// to the top, then End().
    ///
    /// The interface uses the engine theme and font, scaled for the monitor. See ImGuiTheme.
    ///
    /// Docking and multiple viewports are enabled: ImGui windows can be docked into each other or into a dock space,
    /// and dragged outside the main window, where each one gets its own platform window. On Wayland, SDL cannot place
    /// windows, so the viewports are turned off and ImGui windows stay inside the main window. Docking still works.
    ///
    /// While ImGui uses the mouse or the keyboard, for example when the cursor is over an ImGui window or a text field
    /// has the focus, the mouse or keyboard events are marked as handled, so the layers below do not receive them.
    class ImGuiLayer final : public Layer
    {
    public:/// @brief Overlay that starts Dear ImGui with its GLFW and OpenGL backe
        /// @brief Creates the layer for a window that has an OpenGL context. ImGui starts when the layer is attached.
        /// @param window The window where ImGui draws and reads its input. It must outlive the layer.
        explicit ImGuiLayer(Window& window);

        /// @brief Starts ImGui and its SDL and OpenGL backends, and gives ImGui the SDL events of the window.
        void OnAttach() override;

        /// @brief Stops the backends, then ImGui.
        void OnDetach() override;

        /// @brief Marks the mouse and keyboard events as handled while ImGui uses them. See SetBlockEvents().
        void OnEvent(Event& event) override;

        /// @brief Starts a new ImGui frame. The application calls it before the OnImGuiRender() hooks.
        void Begin();

        /// @brief Draws the ImGui frame in the window, then the ImGui windows outside it.
        ///
        /// The application calls it after the OnImGuiRender() hooks.
        void End();

        /// @brief Chooses whether the events used by ImGui are kept from the layers below. True by default.
        ///
        /// The editor turns it off while the cursor is over the scene view, so the scene receives the input.
        void SetBlockEvents(bool block) { m_BlockEvents = block; }

    private:
        Window* m_Window = nullptr;
        bool m_BlockEvents = true;
    };
}
