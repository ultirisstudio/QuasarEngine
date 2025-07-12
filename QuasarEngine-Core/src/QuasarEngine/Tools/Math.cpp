#include "qepch.h"
#include "Math.h"

#include <random>

namespace QuasarEngine {
	Math::Direction Math::AxisToDir(Axis axis, bool negative)
	{
		return Math::Direction(axis * 2 + (negative ? 1 : 0));
	}

	Math::Direction Math::VectorToDir(glm::vec3 vec)
	{
		float max = vec[0];
		int axis = 0;

		for (int i = 1; i < AXIS_COUNT; i++)
		{
			float val = glm::abs(vec[i]);
			if (val > max)
			{
				max = val;
				axis = i;
			}
		}
		return AxisToDir(Axis(axis), vec[axis] < 0.0f);
	}

	float Math::lerp(float a, float b, float x)
	{
		return a + x * (b - a);
	}

	float Math::dist(int x1, int y1, int x2, int y2)
	{
		return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	}

	float Math::random(float min, float max)
	{
		return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
	}

	float Math::random_float()
	{
		static std::default_random_engine e;
		static std::uniform_real_distribution<> dis(-1, 1);
		return dis(e);
	}

	Math::Frustum Math::CalculateFrustum(const glm::mat4& camera)
	{
		Math::Frustum result;
		glm::vec4 base = { camera[0][3], camera[1][3], camera[2][3], camera[3][3] };

		for (unsigned i = 0; i < DIRECTION_COUNT; i++)
		{
			glm::vec4 normal = { camera[0][i / 2], camera[1][i / 2], camera[2][i / 2], camera[3][i / 2] };

			if (i % 2 == 1)
				normal = -1.0f * normal;

			normal += base;
			glm::vec3 eq = normal;
			float len = glm::length(eq);
			eq /= len;
			result.planes[i] = { eq.x, eq.y, eq.z, normal.w / len };
		}

		return result;
	}

	float Math::MapRange(float value, float fromMin, float fromMax, float toMin, float toMax)
	{
		return (((value - fromMin) * (toMax - toMin)) / (fromMax - fromMin)) + toMin;
	}
}

namespace QuasarEngine::Interpolator {
	glm::vec2 CatmullRomInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
	{
		float t2 = t * t;
		float t3 = t2 * t;

		glm::vec2 v0 = (p2 - p0) * 0.5f;
		glm::vec2 v1 = (p3 - p1) * 0.5f;

		float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
		float h01 = -2.0f * t3 + 3.0f * t2;
		float h10 = t3 - 2.0f * t2 + t;
		float h11 = t3 - t2;

		glm::vec2 interpolatedPoint = h00 * p1 + h10 * v0 + h01 * p2 + h11 * v1;

		return interpolatedPoint;
	}

	std::vector<glm::vec2> CatmullRomSplineInterpolation(const std::vector<glm::vec2>& points)
	{
		std::vector<glm::vec2> interpolatedPoints;

		for (size_t i = 1; i < points.size() - 2; ++i)
		{
			const glm::vec2& p0 = points[i - 1];
			const glm::vec2& p1 = points[i];
			const glm::vec2& p2 = points[i + 1];
			const glm::vec2& p3 = points[i + 2];

			for (float t = 0.0f; t <= 1.0f; t += 0.01f)
			{
				glm::vec2 interpolatedPoint = CatmullRomInterpolation(t, p0, p1, p2, p3);
				interpolatedPoints.push_back(interpolatedPoint);
			}
		}

		return interpolatedPoints;
	}

	glm::vec2 CubicInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
	{
		float t2 = t * t;
		float t3 = t2 * t;

		float b0 = 0.5f * (-t3 + 2.0f * t2 - t);
		float b1 = 0.5f * (3.0f * t3 - 5.0f * t2 + 2.0f);
		float b2 = 0.5f * (-3.0f * t3 + 4.0f * t2 + t);
		float b3 = 0.5f * (t3 - t2);

		glm::vec2 interpolatedPoint = b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;

		return interpolatedPoint;
	}

	std::vector<glm::vec2> CubicSplineInterpolation(const std::vector<glm::vec2>& points)
	{
		std::vector<glm::vec2> interpolatedPoints;

		for (size_t i = 0; i < points.size() - 3; ++i)
		{
			const glm::vec2& p0 = points[i];
			const glm::vec2& p1 = points[i + 1];
			const glm::vec2& p2 = points[i + 2];
			const glm::vec2& p3 = points[i + 3];

			for (float t = 0.0f; t <= 1.0f; t += 0.01f)
			{
				glm::vec2 interpolatedPoint = CubicInterpolation(t, p0, p1, p2, p3);
				interpolatedPoints.push_back(interpolatedPoint);
			}
		}

		return interpolatedPoints;
	}

	glm::vec2 BezierInterpolation(float t, const std::vector<glm::vec2>& points)
	{
		int n = points.size() - 1;
		glm::vec2 interpolatedPoint(0.0f);

		for (int i = 0; i <= n; ++i)
		{
			float binomialCoeff = 1.0f;
			float ti = pow(1.0f - t, n - i) * pow(t, i);
			binomialCoeff *= ti;

			interpolatedPoint += binomialCoeff * points[i];
		}

		return interpolatedPoint;
	}

	std::vector<glm::vec2> BezierSplineInterpolation(const std::vector<glm::vec2>& points)
	{
		float numPoints = 1000.0f;

		std::vector<glm::vec2> interpolatedPoints;
		float stepSize = 1.0f / numPoints;

		for (int i = 0; i <= numPoints; ++i)
		{
			float t = i * stepSize;
			glm::vec2 interpolatedPoint = BezierInterpolation(t, points);
			interpolatedPoints.push_back(interpolatedPoint);
		}

		return interpolatedPoints;
	}

	glm::vec2 LinearInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1)
	{
		glm::vec2 interpolatedPoint = p0 + t * (p1 - p0);
		return interpolatedPoint;
	}

	std::vector<glm::vec2> LinearInterpolation(const std::vector<glm::vec2>& points)
	{
		float numPoints = 1000.0f;

		std::vector<glm::vec2> interpolatedPoints;
		int numSegments = points.size() - 1;
		float stepSize = 1.0f / numPoints;

		for (int i = 0; i < numSegments; ++i)
		{
			const glm::vec2& p0 = points[i];
			const glm::vec2& p1 = points[i + 1];

			for (int j = 0; j <= numPoints; ++j)
			{
				float t = j * stepSize;
				glm::vec2 interpolatedPoint = LinearInterpolation(t, p0, p1);
				interpolatedPoints.push_back(interpolatedPoint);
			}
		}

		return interpolatedPoints;
	}
}