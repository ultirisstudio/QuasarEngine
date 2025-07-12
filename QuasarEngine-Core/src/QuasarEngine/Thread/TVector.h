#pragma once

#include <vector>
#include <mutex>

namespace QuasarEngine
{
    template <typename T>
    class TVector {
    private:
        std::vector<T> m_vector;
        mutable std::mutex m_mutex;

    public:
        void push_back(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.push_back(value);
        }

        void pop_back()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_vector.empty())
                m_vector.pop_back();
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector.empty();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector.size();
        }

        std::vector<T> get_copy() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector;
        }
    };
}