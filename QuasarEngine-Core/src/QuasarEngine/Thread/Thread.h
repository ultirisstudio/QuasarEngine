#pragma once

#include <thread>
#include <iostream>

namespace QuasarEngine
{
    class Thread
    {
    public:
        template <typename Callable, typename... Args>
        explicit Thread(Callable&& f, Args&&... args)
        {
            m_thread = std::make_unique<std::thread>(std::forward<Callable>(f), std::forward<Args>(args)...);
        }

        ~Thread()
        {
            if (m_thread && m_thread->joinable())
            {
                m_thread->join();
            }
        }

        void Detach()
        {
            if (m_thread && m_thread->joinable())
            {
                m_thread->detach();
            }
        }

    private:
        std::unique_ptr<std::thread> m_thread;
    };
}