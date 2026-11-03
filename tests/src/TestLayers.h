#pragma once

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Timestep.h>

namespace TestHelpers
{
    class ClosingLayer : public Quasar::Layer
    {
    public:
        explicit ClosingLayer(int closeAfter = 3) : Layer("Closing"), m_CloseAfter(closeAfter) {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++m_Updates;
            if (m_Updates == m_CloseAfter)
            {
                Quasar::Application::Get().Close();
            }
        }

        int GetUpdates() const { return m_Updates; }

    private:
        int m_CloseAfter;
        int m_Updates = 0;
    };
}
