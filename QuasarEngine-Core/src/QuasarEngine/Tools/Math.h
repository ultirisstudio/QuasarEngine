#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace QuasarEngine::Math {
	enum Axis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z,

		AXIS_COUNT
	};

	enum Direction
	{
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_UP,
		DIRECTION_DOWN,
		DIRECTION_FORWARD,
		DIRECTION_BACKWARD,

		DIRECTION_COUNT
	};

	const glm::ivec2 surrounding[] = {
		glm::ivec2(-1.0f, 0.0f),
		glm::ivec2(1.0f,  0.0f),
		glm::ivec2(0.0f,-1.0f),
		glm::ivec2(0.0f, 1.0f),
		glm::ivec2(-1.0f, 1.0f),
		glm::ivec2(1.0f, 1.0f),
		glm::ivec2(1.0f, -1.0f),
		glm::ivec2(-1.0f, -1.0f),
	};

	const glm::vec3 directionVectors[DIRECTION_COUNT]
	{
		{ 1, 0, 0 },
		{ -1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, -1, 0 },
		{ 0, 0, 1 },
		{ 0, 0, -1 },
	};

	struct Plane
	{
		float a, b, c, d;
	};

	struct Frustum
	{
		Plane planes[Math::DIRECTION_COUNT];
	};

	float MapRange(float value, float fromMin, float fromMax, float toMin, float toMax);
	float lerp(float a, float b, float x);
	float dist(int x1, int y1, int x2, int y2);
	float random(float min, float max);
	float random_float();

	Direction AxisToDir(Axis axis, bool negative);
	Direction VectorToDir(glm::vec3 vec);

	Frustum CalculateFrustum(const glm::mat4& camera);
}

namespace QuasarEngine::Interpolator {
	std::vector<glm::vec2> CatmullRomSplineInterpolation(const std::vector<glm::vec2>& points);
	std::vector<glm::vec2> CubicSplineInterpolation(const std::vector<glm::vec2>& points);
	std::vector<glm::vec2> BezierSplineInterpolation(const std::vector<glm::vec2>& points);
	std::vector<glm::vec2> LinearInterpolation(const std::vector<glm::vec2>& points);

	glm::vec2 CatmullRomInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);
	glm::vec2 CubicInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);
	glm::vec2 BezierInterpolation(float t, const std::vector<glm::vec2>& points);
	glm::vec2 LinearInterpolation(float t, const glm::vec2& p0, const glm::vec2& p1);
}