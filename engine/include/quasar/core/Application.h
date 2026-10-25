#pragma once

/// \file
/// \brief The application: it creates the window, owns the layers and runs the main loop.
/// \author Kurogenshi
/// \date 2026-10-24

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/core/LayerStack.h>
#include <quasar/core/Timer.h>
#include <quasar/core/Window.h>
#include <quasar/events/Event.h>

namespace Quasar
{
    /// @brief Settings used to create the application.
    struct ApplicationSpecification
    {
        WindowSpecification Window; ///< Settings of the main window, created with the application.
    };

    /// @brief The application: it creates the window, owns the layers and runs the main loop.
    ///
    /// An application is built by adding layers to it, then calling Run(). Only one application can exist at a time,
    /// and it can be reached from anywhere with Get().
    class Application
    {
    public:
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        /// @brief Creates the application and its window.
        /// @param specification Settings of the application and of its window.
        /// @return The new application, or `nullptr` if an application already exists or if the window cannot be
        /// created. The reason is logged as an engine error.
        [[nodiscard]] static Scope<Application> Create(const ApplicationSpecification& specification);

        /// @brief Returns the application. An application must exist.
        static Application& Get();

        /// @brief Runs the main loop until the window is closed or Close() is called.
        ///
        /// Each frame, the loop computes the time elapsed since the previous frame, updates every layer, renders
        /// every layer, then shows the image and processes the messages of the operating system. Layers are updated
        /// and rendered from the bottom of the stack to the top. While the window is minimized, the layers are still
        /// updated but not rendered.
        ///
        /// The events of the window go to the application first, then to the layers from the top of the stack to
        /// the bottom, until a layer handles them. Closing the window stops the loop.
        void Run();

        /// @brief Asks the main loop to stop at the end of the current frame.
        void Close();

        /// @brief Adds a regular layer above the other regular layers. See LayerStack::PushLayer().
        Layer& PushLayer(Scope<Layer> layer);

        /// @brief Adds an overlay above every layer. See LayerStack::PushOverlay().
        Layer& PushOverlay(Scope<Layer> overlay);

        /// @brief Removes a regular layer. See LayerStack::PopLayer().
        Scope<Layer> PopLayer(Layer& layer);

        /// @brief Removes an overlay. See LayerStack::PopOverlay().
        Scope<Layer> PopOverlay(Layer& overlay);

        /// @brief Returns the main window.
        Window& GetWindow() { return *m_Window; }

    private:
        explicit Application(Scope<Window> window);

        void OnEvent(Event& event);

        Scope<Window> m_Window;
        LayerStack m_LayerStack;
        Timer m_FrameTimer;
        double m_LastFrameTime = 0.0;
        bool m_Running = false;
        bool m_Minimized = false;
    };
}
