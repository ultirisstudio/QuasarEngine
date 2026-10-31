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

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Quasar
{
    /// @brief Settings used to create the application.
    struct ApplicationSpecification
    {
        std::string Name = "QuasarEngine"; ///< Name of the application, used in the logs.

        /// Directory used as the current directory, so that relative paths start from it. A relative directory starts
        /// from the directory the program was started from. Empty keeps the current directory.
        std::filesystem::path WorkingDirectory{};

        std::vector<std::string> Arguments{}; ///< Command-line arguments, without the name of the program.
        WindowSpecification Window{};         ///< Settings of the main window, created with the application.
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
        /// @return The new application, or `nullptr` if an application already exists, if the working directory
        /// cannot be used or if the window cannot be created. The reason is logged as an engine error.
        [[nodiscard]] static Scope<Application> Create(const ApplicationSpecification& specification);

        /// @brief Returns the application. An application must exist.
        static Application& Get();

        /// @brief Runs the main loop until the window is closed or Close() is called.
        ///
        /// Each frame, the loop computes the time elapsed since the previous frame, updates every layer, renders
        /// every layer, then shows the image and processes the messages of the operating system. Layers are updated
        /// and rendered from the bottom of the stack to the top. While the window is minimized, or while its drawing
        /// area is empty, the layers are still updated but not rendered.
        ///
        /// The events of the window go to the application first, then to the layers from the top of the stack to
        /// the bottom, until a layer handles them. Closing the window stops the loop.
        ///
        /// Layers added or removed during a frame, by a layer or by an event, are added or removed at the end of the
        /// frame, in the order they were asked for. The layers being updated, rendered or receiving an event never
        /// change while they are being gone through.
        void Run();

        /// @brief Asks the main loop to stop at the end of the current frame.
        void Close();

        /// @brief Adds a regular layer above the other regular layers. See LayerStack::PushLayer().
        ///
        /// During the main loop, the layer is added and attached at the end of the current frame. Otherwise it is
        /// added right away.
        /// @param layer The layer to add. It must not be empty.
        /// @return The layer, which stays at the same address until it is removed.
        Layer& PushLayer(Scope<Layer> layer);

        /// @brief Adds an overlay above every layer. See LayerStack::PushOverlay().
        ///
        /// During the main loop, the overlay is added and attached at the end of the current frame. Otherwise it is
        /// added right away.
        /// @param overlay The overlay to add. It must not be empty.
        /// @return The overlay, which stays at the same address until it is removed.
        Layer& PushOverlay(Scope<Layer> overlay);

        /// @brief Detaches, removes and destroys a regular layer. See LayerStack::PopLayer().
        ///
        /// During the main loop, this happens at the end of the current frame, so a layer can remove itself.
        /// Otherwise it happens right away.
        /// @param layer A regular layer of this application.
        void PopLayer(Layer& layer);

        /// @brief Detaches, removes and destroys an overlay. See LayerStack::PopOverlay().
        ///
        /// During the main loop, this happens at the end of the current frame, so an overlay can remove itself.
        /// Otherwise it happens right away.
        /// @param overlay An overlay of this application.
        void PopOverlay(Layer& overlay);

        /// @brief Returns the main window.
        Window& GetWindow() { return *m_Window; }

        /// @brief Returns the settings the application was created with, including the command-line arguments.
        const ApplicationSpecification& GetSpecification() const { return m_Specification; }

    private:
        Application(ApplicationSpecification specification, Scope<Window> window);

        struct LayerChange
        {
            enum class Type : std::uint8_t
            {
                PushLayer,
                PushOverlay,
                PopLayer,
                PopOverlay
            };

            Type Kind{};
            Scope<Layer> Added{};
            Layer* Removed = nullptr;
        };

        void OnEvent(Event& event);
        void ApplyLayerChanges();

        ApplicationSpecification m_Specification;

        Scope<Window> m_Window;
        LayerStack m_LayerStack;
        std::vector<LayerChange> m_LayerChanges;
        Timer m_FrameTimer;
        double m_LastFrameTime = 0.0;
        bool m_Running = false;
        bool m_Minimized = false;
        bool m_InMainLoop = false;
    };
}
