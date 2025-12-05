#pragma once

#include <thread>
#include <utility>

namespace QuasarEngine
{
    class Thread
    {
    public:
        Thread() = default;

        template<typename Callable, typename... Args>
        explicit Thread(Callable&& f, Args&&... args)
            : m_thread(std::forward<Callable>(f), std::forward<Args>(args)...)
        {
        }

        ~Thread()
        {
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        Thread(Thread&& other) noexcept = default;
        Thread& operator=(Thread&& other) noexcept = default;

        bool Joinable() const noexcept
        {
            return m_thread.joinable();
        }

        void Join()
        {
            if (m_thread.joinable())
                m_thread.join();
        }

        void Detach()
        {
            if (m_thread.joinable())
                m_thread.detach();
        }

    private:
        std::thread m_thread;
    };
}