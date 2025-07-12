#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class Curve {
    public:
        void AddPoint(glm::vec2 pt);
        void RemovePoint(size_t idx);
        void MovePoint(size_t idx, glm::vec2 pt);
        size_t PointCount() const { return m_Points.size(); }
        std::vector<glm::vec2>& GetPoints() { return m_Points; }
        std::vector<glm::vec2> GetSortedPoints() const;
        std::vector<glm::vec2> InterpolateCurve(const std::vector<glm::vec2>& sorted, int steps) const;
        float Evaluate(float x, const std::vector<glm::vec2>& sorted) const;
    private:
        std::vector<glm::vec2> m_Points;
    };
}