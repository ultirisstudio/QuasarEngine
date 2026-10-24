#include <quasar/core/LayerStack.h>
#include <quasar/core/Log.h>

#include <doctest/doctest.h>

#include <ranges>
#include <string>
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