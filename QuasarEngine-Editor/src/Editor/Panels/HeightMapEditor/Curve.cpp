#include "Curve.h"
#include <algorithm>

namespace QuasarEngine
{
    static inline glm::vec2 ClampPoint(const glm::vec2& p) {
        return { glm::clamp(p.x, Curve::kXMin, Curve::kXMax),
                 glm::clamp(p.y, Curve::kYMin, Curve::kYMax) };
    }

    void Curve::AddPoint(glm::vec2 pt)
    {
        m_Points.push_back(ClampPoint(pt));
    }

    void Curve::RemovePoint(std::size_t idx)
    {
        if (idx < m_Points.size())
            m_Points.erase(m_Points.begin() + static_cast<std::ptrdiff_t>(idx));
    }

    void Curve::MovePoint(std::size_t idx, glm::vec2 pt)
    {
        if (idx < m_Points.size())
            m_Points[idx] = ClampPoint(pt);
    }

    void Curve::Clear()
    {
        m_Points.clear();
    }

    void Curve::SetPoints(const std::vector<glm::vec2>& pts)
    {
        m_Points.clear();
        m_Points.reserve(pts.size());
        for (const auto& p : pts)
            m_Points.emplace_back(ClampPoint(p));
    }

    std::vector<glm::vec2> Curve::GetSortedPoints() const
    {
        auto tmp = m_Points;
        std::stable_sort(tmp.begin(), tmp.end(),
            [](const glm::vec2& a, const glm::vec2& b) {
                return a.x < b.x;
            });
        return tmp;
    }

    std::vector<glm::vec2> Curve::InterpolateCurve(const std::vector<glm::vec2>& sorted, int steps) const
    {
        std::vector<glm::vec2> result;
        if (sorted.size() < 2 || steps < 2) return result;
        result.reserve(static_cast<std::size_t>(steps));

        for (int i = 0; i < steps; ++i)
        {
            float x = (steps == 1) ? 0.0f : (float(i) / float(steps - 1));
            float y = Evaluate(x, sorted);
            result.push_back({ x, y });
        }
        return result;
    }

    float Curve::Evaluate(float x, const std::vector<glm::vec2>& sorted) const
    {
        if (sorted.empty()) return 0.0f;
        if (sorted.size() == 1) return glm::clamp(sorted.front().y, kYMin, kYMax);

        x = glm::clamp(x, kXMin, kXMax);

        if (x <= sorted.front().x) return glm::clamp(sorted.front().y, kYMin, kYMax);
        if (x >= sorted.back().x)  return glm::clamp(sorted.back().y, kYMin, kYMax);

        auto it = std::lower_bound(sorted.begin(), sorted.end(), x,
            [](const glm::vec2& p, float val) {
                return p.x < val;
            });

        if (it != sorted.end() && it->x == x)
            return glm::clamp(it->y, kYMin, kYMax);

        auto it1 = it;
        auto it0 = it - 1;

        float dx = it1->x - it0->x;
        if (dx <= 1e-8f)
            return glm::clamp(it0->y, kYMin, kYMax);

        float t = (x - it0->x) / dx;
        float y = glm::mix(it0->y, it1->y, t);

        return glm::clamp(y, kYMin, kYMax);
    }
}
