#pragma once

#include <stack>
#include <mutex>
#include <optional>

namespace QuasarEngine
{
    template <typename T>
    class TStack {
    private:
        std::stack<T> m_stack;
        mutable std::mutex m_mutex;

    public:
        void push(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stack.push(value);
        }

        std::optional<T> pop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stack.empty())
                return std::nullopt;

            T value = m_stack.top();
            m_stack.pop();
            return value;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_stack.empty();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_stack.size();
        }
    };
}