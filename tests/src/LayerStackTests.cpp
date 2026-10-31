#include <quasar/core/LayerStack.h>
#include <quasar/core/Log.h>
#include <quasar/events/WindowEvent.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <random>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class RecordingLayer : public Quasar::Layer
    {
    public:
        RecordingLayer(std::string name, std::vector<std::string>& calls) : Layer(std::move(name)), m_Calls(calls) {}

        void OnAttach() override { m_Calls.push_back("attach " + GetName()); }
        void OnDetach() override { m_Calls.push_back("detach " + GetName()); }

    private:
        std::vector<std::string>& m_Calls;
    };

    class EventLayer : public Quasar::Layer
    {
    public:
        EventLayer(std::string name, std::vector<std::string>& calls, bool handlesEvents)
            : Layer(std::move(name)), m_Calls(calls), m_HandlesEvents(handlesEvents)
        {
        }

        void OnEvent(Quasar::Event& event) override
        {
            m_Calls.push_back(GetName());
            if (m_HandlesEvents)
            {
                event.SetHandled(true);
            }
        }

    private:
        std::vector<std::string>& m_Calls;
        bool m_HandlesEvents = false;
    };

    Quasar::Scope<Quasar::Layer> MakeLayer(std::string name, std::vector<std::string>& calls)
    {
        return Quasar::CreateScope<RecordingLayer>(std::move(name), calls);
    }

    std::vector<std::string> GetNames(const Quasar::LayerStack& stack)
    {
        std::vector<std::string> names;
        for (const Quasar::Scope<Quasar::Layer>& layer : stack)
        {
            names.push_back(layer->GetName());
        }
        return names;
    }
}

TEST_CASE("Layers and overlays can be added and removed")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    Quasar::Layer& game = stack.PushLayer(MakeLayer("Game", calls));
    Quasar::Layer& editor = stack.PushOverlay(MakeLayer("Editor", calls));
    CHECK(stack.GetSize() == 2);

    const Quasar::Scope<Quasar::Layer> removedGame = stack.PopLayer(game);
    const Quasar::Scope<Quasar::Layer> removedEditor = stack.PopOverlay(editor);

    REQUIRE(removedGame != nullptr);
    REQUIRE(removedEditor != nullptr);
    CHECK(removedGame->GetName() == "Game");
    CHECK(removedEditor->GetName() == "Editor");
    CHECK(stack.GetSize() == 0);
}

TEST_CASE("Overlays stay above regular layers whatever the order they were added in")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    stack.PushOverlay(MakeLayer("Overlay 1", calls));
    stack.PushLayer(MakeLayer("Layer 1", calls));
    stack.PushOverlay(MakeLayer("Overlay 2", calls));
    stack.PushLayer(MakeLayer("Layer 2", calls));

    const std::vector<std::string> expected{"Layer 1", "Layer 2", "Overlay 1", "Overlay 2"};
    CHECK(GetNames(stack) == expected);
}

TEST_CASE("Overlays stay above regular layers after any sequence of additions and removals")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    std::vector<std::pair<std::string, Quasar::Layer*>> layers;
    std::vector<std::pair<std::string, Quasar::Layer*>> overlays;

    // A fixed seed gives the same sequence on every run, so a failure can be reproduced
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp)
    std::mt19937 random(2026);
    const auto pick = [&random](std::size_t count)
    { return std::uniform_int_distribution<std::size_t>(0, count - 1)(random); };

    constexpr int steps = 2000;
    for (int step = 0; step < steps; ++step)
    {
        CAPTURE(step);
        const std::string name = "Layer " + std::to_string(step);

        switch (pick(4))
        {
            case 0:
                layers.emplace_back(name, &stack.PushLayer(MakeLayer(name, calls)));
                break;
            case 1:
                overlays.emplace_back(name, &stack.PushOverlay(MakeLayer(name, calls)));
                break;
            case 2:
                if (!layers.empty())
                {
                    const auto removed = layers.begin() + static_cast<std::ptrdiff_t>(pick(layers.size()));
                    REQUIRE(stack.PopLayer(*removed->second) != nullptr);
                    layers.erase(removed);
                }
                break;
            default:
                if (!overlays.empty())
                {
                    const auto removed = overlays.begin() + static_cast<std::ptrdiff_t>(pick(overlays.size()));
                    REQUIRE(stack.PopOverlay(*removed->second) != nullptr);
                    overlays.erase(removed);
                }
                break;
        }

        std::vector<std::string> expected;
        expected.reserve(layers.size() + overlays.size());
        for (const auto& [layerName, layer] : layers)
        {
            expected.push_back(layerName);
        }
        for (const auto& [overlayName, overlay] : overlays)
        {
            expected.push_back(overlayName);
        }
        REQUIRE(GetNames(stack) == expected);
    }
}

TEST_CASE("Removing a layer keeps the other layers in order")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    stack.PushLayer(MakeLayer("Layer 1", calls));
    Quasar::Layer& middle = stack.PushLayer(MakeLayer("Layer 2", calls));
    stack.PushOverlay(MakeLayer("Overlay", calls));
    stack.PushLayer(MakeLayer("Layer 3", calls));

    const Quasar::Scope<Quasar::Layer> removed = stack.PopLayer(middle);
    stack.PushLayer(MakeLayer("Layer 4", calls));

    const std::vector<std::string> expected{"Layer 1", "Layer 3", "Layer 4", "Overlay"};
    CHECK(GetNames(stack) == expected);
}

TEST_CASE("Iterating in reverse goes from the top layer to the bottom one")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    stack.PushLayer(MakeLayer("Layer", calls));
    stack.PushOverlay(MakeLayer("Overlay", calls));

    std::vector<std::string> names;
    for (const Quasar::Scope<Quasar::Layer>& layer : std::views::reverse(stack))
    {
        names.push_back(layer->GetName());
    }

    const std::vector<std::string> expected{"Overlay", "Layer"};
    CHECK(names == expected);
}

TEST_CASE("Layers are attached when added and detached when removed")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;

    Quasar::Layer& game = stack.PushLayer(MakeLayer("Game", calls));
    Quasar::Layer& editor = stack.PushOverlay(MakeLayer("Editor", calls));
    const Quasar::Scope<Quasar::Layer> removedEditor = stack.PopOverlay(editor);
    const Quasar::Scope<Quasar::Layer> removedGame = stack.PopLayer(game);

    const std::vector<std::string> expected{"attach Game", "attach Editor", "detach Editor", "detach Game"};
    CHECK(calls == expected);
}

TEST_CASE("Destroying the stack detaches every layer from the top to the bottom")
{
    std::vector<std::string> calls;
    {
        Quasar::LayerStack stack;
        stack.PushOverlay(MakeLayer("Overlay", calls));
        stack.PushLayer(MakeLayer("Layer", calls));
        calls.clear();
    }

    const std::vector<std::string> expected{"detach Overlay", "detach Layer"};
    CHECK(calls == expected);
}

TEST_CASE("Popping a layer that is not in the stack does nothing")
{
    Quasar::Log::Init();

    std::vector<std::string> calls;
    Quasar::LayerStack stack;
    Quasar::Layer& layer = stack.PushLayer(MakeLayer("Layer", calls));
    Quasar::Layer& overlay = stack.PushOverlay(MakeLayer("Overlay", calls));
    calls.clear();

    CHECK(stack.PopOverlay(layer) == nullptr);
    CHECK(stack.PopLayer(overlay) == nullptr);
    CHECK(stack.GetSize() == 2);
    CHECK(calls.empty());

    Quasar::Log::Shutdown();
}

TEST_CASE("Events go from the top layer down and stop once handled")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;
    stack.PushLayer(Quasar::CreateScope<EventLayer>("Game", calls, false));
    stack.PushLayer(Quasar::CreateScope<EventLayer>("Scene", calls, true));
    stack.PushOverlay(Quasar::CreateScope<EventLayer>("Editor", calls, false));
    Quasar::WindowResizeEvent event(800, 600);

    stack.OnEvent(event);

    const std::vector<std::string> expected{"Editor", "Scene"};
    CHECK(calls == expected);
    CHECK(event.IsHandled());
}

TEST_CASE("An event that no layer handles reaches every layer")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;
    stack.PushLayer(Quasar::CreateScope<EventLayer>("Game", calls, false));
    stack.PushOverlay(Quasar::CreateScope<EventLayer>("Editor", calls, false));
    Quasar::WindowResizeEvent event(800, 600);

    stack.OnEvent(event);

    const std::vector<std::string> expected{"Editor", "Game"};
    CHECK(calls == expected);
    CHECK_FALSE(event.IsHandled());
}

TEST_CASE("An event that is already handled reaches no layer")
{
    std::vector<std::string> calls;
    Quasar::LayerStack stack;
    stack.PushLayer(Quasar::CreateScope<EventLayer>("Game", calls, false));
    Quasar::WindowResizeEvent event(800, 600);
    event.SetHandled(true);

    stack.OnEvent(event);

    CHECK(calls.empty());
}
