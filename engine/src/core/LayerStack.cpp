#include <quasar/core/LayerStack.h>
#include <quasar/core/Log.h>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace Quasar
{
    namespace
    {
        using LayerIterator = std::vector<Scope<Layer>>::iterator;

        LayerIterator Find(LayerIterator first, LayerIterator last, const Layer& layer)
        {
            return std::find_if(first, last, [&](const Scope<Layer>& current) { return current.get() == &layer; });
        }
    }

    LayerStack::~LayerStack()
    {
        for (Scope<Layer>& layer : std::views::reverse(m_Layers))
        {
            layer->OnDetach();
        }
    }

    Layer& LayerStack::PushLayer(Scope<Layer> layer)
    {
        QUASAR_ENGINE_ASSERT(layer != nullptr, "Cannot push an empty layer");

        const auto position = m_Layers.begin() + static_cast<std::ptrdiff_t>(m_OverlayStart);
        Layer& added = **m_Layers.insert(position, std::move(layer));
        ++m_OverlayStart;

        added.OnAttach();
        return added;
    }

    Layer& LayerStack::PushOverlay(Scope<Layer> overlay)
    {
        QUASAR_ENGINE_ASSERT(overlay != nullptr, "Cannot push an empty overlay");

        Layer& added = *m_Layers.emplace_back(std::move(overlay));

        added.OnAttach();
        return added;
    }

    Scope<Layer> LayerStack::PopLayer(Layer& layer)
    {
        const auto overlayStart = m_Layers.begin() + static_cast<std::ptrdiff_t>(m_OverlayStart);
        const auto found = Find(m_Layers.begin(), overlayStart, layer);
        if (found == overlayStart)
        {
            QUASAR_ENGINE_WARN("Cannot pop the layer '{}': it is not a regular layer of this stack", layer.GetName());
            return nullptr;
        }

        layer.OnDetach();
        Scope<Layer> removed = std::move(*found);
        m_Layers.erase(found);
        --m_OverlayStart;
        return removed;
    }

    Scope<Layer> LayerStack::PopOverlay(Layer& overlay)
    {
        const auto overlayStart = m_Layers.begin() + static_cast<std::ptrdiff_t>(m_OverlayStart);
        const auto found = Find(overlayStart, m_Layers.end(), overlay);
        if (found == m_Layers.end())
        {
            QUASAR_ENGINE_WARN("Cannot pop the overlay '{}': it is not an overlay of this stack", overlay.GetName());
            return nullptr;
        }

        overlay.OnDetach();
        Scope<Layer> removed = std::move(*found);
        m_Layers.erase(found);
        return removed;
    }
}
