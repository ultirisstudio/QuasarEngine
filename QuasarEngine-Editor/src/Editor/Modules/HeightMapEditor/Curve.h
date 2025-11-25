#pragma once

#include <vector>
#include <cstddef>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class Curve {
    public:
        static constexpr float kXMin = 0.0f;
        static constexpr float kXMax = 1.0f;
        static constexpr float kYMin = 0.0f;
        static constexpr float kYMax = 255.0f;

        void AddPoint(glm::vec2 pt);
        void RemovePoint(std::size_t idx);
        void MovePoint(std::size_t idx, glm::vec2 pt);
        void Clear();
        void SetPoints(const std::vector<glm::vec2>& pts);

        std::size_t PointCount() const { return m_Points.size(); }
        std::vector<glm::vec2>& GetPoints() { return m_Points; }
        const std::vector<glm::vec2>& GetPoints() const { return m_Points; }

        std::vector<glm::vec2> GetSortedPoints() const;

        std::vector<glm::vec2> InterpolateCurve(const std::vector<glm::vec2>& sorted, int steps) const;

        float Evaluate(float x, const std::vector<glm::vec2>& sorted) const;

    private:
        static inline float ClampX(float x) { return glm::clamp(x, kXMin, kXMax); }
        static inline float ClampY(float y) { return glm::clamp(y, kYMin, kYMax); }

        std::vector<glm::vec2> m_Points;
    };
}
