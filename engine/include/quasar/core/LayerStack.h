#pragma once

/// \file
/// \brief Ordered list of the layers of an application, with overlays always above regular layers.
/// \author Kurogenshi
/// \date 2026-10-24

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>

#include <cstddef>
#include <vector>

namespace Quasar
{
    /// @brief Owns the layers of an application and keeps them in order, from the bottom to the top.
    ///
    /// Regular layers, like the game scene, are at the bottom. Overlays, like the editor panels or a debug view, are
    /// always above every regular layer, whatever the order they were added in. Each kind keeps its own order: a new
    /// layer goes above the other layers, and a new overlay above the other overlays.
    ///
    /// Iterating over the stack goes from the bottom to the top, which is the order for updating and rendering. Use
    /// `std::views::reverse(stack)` to go from the top to the bottom, which is the order for events. Do not add or
    /// remove layers while iterating.
    class LayerStack
    {
    public:
        /// @brief Iterator over the layers, from the bottom to the top.
        using Iterator = std::vector<Scope<Layer>>::iterator;

        /// @brief Iterator over the layers, from the bottom to the top, without changing them.
        using ConstIterator = std::vector<Scope<Layer>>::const_iterator;
        
        LayerStack() = default;

        /// @brief Detaches every layer, from the top to the bottom, then destroys them.
        ~LayerStack();

        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;
        LayerStack(LayerStack&&) = delete;
        LayerStack& operator=(LayerStack&&) = delete;

        /// @brief Adds a regular layer above the other regular layers and below every overlay, then attaches it.
        /// @param layer The layer to add. It must not be empty.
        /// @return The added layer, which stays owned by the stack.
        Layer& PushLayer(Scope<Layer> layer);

        /// @brief Adds an overlay above every layer and overlay, then attaches it.
        /// @param overlay The overlay to add. It must not be empty.
        /// @return The added overlay, which stays owned by the stack.
        Layer& PushOverlay(Scope<Layer> overlay);

        /// @brief Detaches a regular layer and removes it from the stack.
        /// @return The removed layer, now owned by the caller, or an empty Scope if it is not a regular layer of this
        /// stack. In that case, a warning is logged.
        Scope<Layer> PopLayer(Layer& layer);

        /// @brief Detaches an overlay and removes it from the stack.
        /// @return The removed overlay, now owned by the caller, or an empty Scope if it is not an overlay of this
        /// stack. In that case, a warning is logged.
        Scope<Layer> PopOverlay(Layer& overlay);

        /// @brief Returns the number of layers and overlays in the stack.
        std::size_t GetSize() const { return m_Layers.size(); }

        /// @brief Returns an iterator to the bottom layer.
        Iterator begin() { return m_Layers.begin(); }

        /// @brief Returns an iterator past the top layer.
        Iterator end() { return m_Layers.end(); }

        /// @brief Returns an iterator to the bottom layer.
        ConstIterator begin() const { return m_Layers.begin(); }

        /// @brief Returns an iterator past the top layer.
        ConstIterator end() const { return m_Layers.end(); }

    private:
        std::vector<Scope<Layer>> m_Layers;
        std::size_t m_OverlayStart = 0;
    };
}