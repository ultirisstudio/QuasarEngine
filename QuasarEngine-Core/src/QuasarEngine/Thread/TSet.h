#pragma once

#include <set>
#include <mutex>

namespace QuasarEngine
{
    template <typename T>
    class TSet {
    private:
        std::set<T> m_set;
        mutable std::mutex m_mutex;

    public:
        void insert(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_set.insert(value);
        }

        void erase(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_set.erase(value);
        }

        bool contains(const T& value) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_set.find(value) != m_set.end();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_set.size();
        }
    };
}