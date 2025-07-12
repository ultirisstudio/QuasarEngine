#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace QuasarEngine
{
    template <typename T>
    class TQueue {
    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_cond;

    public:
        void push(const T& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(item);
            m_cond.notify_one();
        }

        std::optional<T> front() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return std::nullopt;
            return m_queue.front();
        }

        T wait_and_pop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] { return !m_queue.empty(); });

            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        std::optional<T> try_pop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return std::nullopt;

            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }
    };
}