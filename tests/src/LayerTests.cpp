#include <quasar/core/Layer.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/WindowEvent.h>

#include <doctest/doctest.h>

#include <type_traits>

namespace
{
    class EmptyLayer : public Quasar::Layer
    {
    };

    class UpdateLayer : public Quasar::Layer
    {
    public:
        UpdateLayer() : Layer("Update layer") {}

        void OnUpdate(Quasar::Timestep timestep) override { m_TotalSeconds += timestep.GetSeconds(); }

        float GetTotalSeconds() const { return m_TotalSeconds; }

    private:
        float m_TotalSeconds = 0.0f;
    };

    class CloseLayer : public Quasar::Layer
    {
    public:
        CloseLayer() : Layer("Close layer") {}

        void OnEvent(Quasar::Event& event) override
        {
            Quasar::EventDispatcher dispatcher(event);
            dispatcher.Dispatch<Quasar::WindowCloseEvent>([](Quasar::WindowCloseEvent&) { return true; });
        }
    };

    static_assert(std::has_virtual_destructor_v<Quasar::Layer>, "Layers are destroyed through a Layer pointer");
    static_assert(!std::is_copy_constructible_v<Quasar::Layer>);
    static_assert(!std::is_move_constructible_v<Quasar::Layer>);
}

TEST_CASE("A layer has a name")
{
    const EmptyLayer unnamed;
    const UpdateLayer named;

    CHECK(unnamed.GetName() == "Layer");
    CHECK(named.GetName() == "Update layer");
}

TEST_CASE("A layer that overrides no hook does nothing")
{
    EmptyLayer layer;
    Quasar::Layer& base = layer;
    Quasar::WindowCloseEvent event;

    base.OnAttach();
    base.OnUpdate(Quasar::Timestep(0.016f));
    base.OnRender();
    base.OnImGuiRender();
    base.OnEvent(event);
    base.OnDetach();

    CHECK_FALSE(event.IsHandled());
}

TEST_CASE("A layer only overrides the hooks it needs")
{
    UpdateLayer layer;
    Quasar::Layer& base = layer;
    Quasar::WindowCloseEvent event;

    base.OnUpdate(Quasar::Timestep(0.25f));
    base.OnUpdate(Quasar::Timestep(0.5f));
    base.OnEvent(event);

    CHECK(layer.GetTotalSeconds() == doctest::Approx(0.75));
    CHECK_FALSE(event.IsHandled());
}

TEST_CASE("A layer can handle the events it receives")
{
    CloseLayer layer;
    Quasar::Layer& base = layer;
    Quasar::WindowCloseEvent close;
    Quasar::WindowResizeEvent resize(800, 600);

    base.OnEvent(close);
    base.OnEvent(resize);

    CHECK(close.IsHandled());
    CHECK_FALSE(resize.IsHandled());
}