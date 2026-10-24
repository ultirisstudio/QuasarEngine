#pragma once

/// \file
/// \brief Base class for every part of an application that is updated, rendered and receives events.
/// \author Kurogenshi
/// \date 2026-10-24

#include <quasar/core/Timestep.h>
#include <quasar/events/Event.h>

#include <string>
#include <utility>

namespace Quasar
{
    /// @brief Part of an application that is updated, rendered and receives events.
    ///
    /// An application is made of layers, for example the game scene, then the editor panels on top of it. Each
    /// layer derives from this class and only overrides the hooks it needs: all of them do nothing by default.
    ///
    /// A layer cannot be copied or moved, because the application keeps it at the same address while it is attached.
    class Layer
    {
    public:
        virtual ~Layer() = default;

        Layer(const Layer&) = delete;
        Layer& operator=(const Layer&) = delete;
        Layer(Layer&&) = delete;
        Layer& operator=(Layer&&) = delete;

        /// @brief Called once when the layer is added to the application, before any other hook.
        virtual void OnAttach() {}

        /// @brief Called once when the layer is removed from the application, after every other hook.
        virtual void OnDetach() {}

        /// @brief Called once per frame to update the layer.
        /// @param timestep Time elapsed since the previous frame.
        virtual void OnUpdate([[maybe_unused]] Timestep timestep) {}

        /// @brief Called once per frame, after OnUpdate(), to render the layer.
        virtual void OnRender() {}

        /// @brief Called once per frame, after OnRender(), to draw the ImGui windows of the layer.
        virtual void OnImGuiRender() {}

        /// @brief Called for each event received by the layer.
        ///
        /// Mark the event as handled to stop it from reaching the layers below this one.
        virtual void OnEvent([[maybe_unused]] Event& event) {}

        /// @brief Returns the name of the layer, for debugging.
        const std::string& GetName() const { return m_Name; }

    protected:
        /// @brief Creates a layer with a name, used in logs and debugging tools.
        explicit Layer(std::string name = "Layer") : m_Name(std::move(name)) {}

    private:
        std::string m_Name;
    };
}