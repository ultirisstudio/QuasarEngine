#pragma once

#include <unordered_map>
#include <mutex>
#include <optional>

namespace QuasarEngine
{
    template <typename K, typename V>
    class TUnorderedMap {
    private:
        std::unordered_map<K, V> m_map;
        mutable std::mutex m_mutex;

    public:
        void insert(const K& key, const V& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_map[key] = value;
        }

        std::optional<V> get(const K& key) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_map.find(key);
            if (it != m_map.end())
                return it->second;
            return std::nullopt;
        }

        void erase(const K& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_map.erase(key);
        }

        bool contains(const K& key) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_map.find(key) != m_map.end();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_map.size();
        }
    };
}