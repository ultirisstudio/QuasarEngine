#include "Curve.h"
#include <algorithm>

namespace QuasarEngine
{
    void Curve::AddPoint(glm::vec2 pt) { m_Points.push_back(pt); }
    void Curve::RemovePoint(size_t idx) { if (idx < m_Points.size()) m_Points.erase(m_Points.begin() + idx); }
    void Curve::MovePoint(size_t idx, glm::vec2 pt) { if (idx < m_Points.size()) m_Points[idx] = pt; }

    std::vector<glm::vec2> Curve::GetSortedPoints() const {
        auto tmp = m_Points;
        std::sort(tmp.begin(), tmp.end(), [](const glm::vec2& a, const glm::vec2& b) { return a.x < b.x; });
        return tmp;
    }

    std::vector<glm::vec2> Curve::InterpolateCurve(const std::vector<glm::vec2>& sorted, int steps) const {
        std::vector<glm::vec2> result;
        if (sorted.size() < 2) return result;
        for (int i = 0; i < steps; ++i) {
            float x = float(i) / (steps - 1);
            float y = Evaluate(x, sorted);
            result.push_back({ x, y });
        }
        return result;
    }

    float Curve::Evaluate(float x, const std::vector<glm::vec2>& sorted) const {
        if (sorted.size() < 2) return 0.f;
        if (x <= sorted.front().x) return sorted.front().y;
        if (x >= sorted.back().x) return sorted.back().y;
        for (size_t i = 1; i < sorted.size(); ++i) {
            if (x < sorted[i].x) {
                float t = (x - sorted[i - 1].x) / (sorted[i].x - sorted[i - 1].x);
                return glm::mix(sorted[i - 1].y, sorted[i].y, t);
            }
        }
        return sorted.back().y;
    }
}