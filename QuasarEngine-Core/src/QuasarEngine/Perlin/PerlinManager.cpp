#include "qepch.h"
#include "PerlinManager.h"

#include <glm/glm.hpp>

#include <QuasarEngine/Core/UUID.h>
#include <QuasarEngine/Tools/PerlinNoise.h>

#include <QuasarEngine/Tools/Math.h>

namespace QuasarEngine
{
	namespace Utils {
		const glm::vec2& GetLeftPoint(const std::vector<glm::vec2>& points, float x)
		{
			unsigned int index = 0;

			for (int i = 0; i < points.size() - 1; i++)
			{
				if (points[i].x < x)
					index = i;
				else
					break;
			}

			return points[index];
		}

		const glm::vec2& GetRightPoint(const std::vector<glm::vec2>& points, float x)
		{
			unsigned int index = static_cast<unsigned int>(points.size() - 1);

			for (int i = static_cast<int>(points.size()) - 1; i >= 0; i--)
			{
				if (points[i].x > x)
					index = i;
				else
					break;
			}

			return points[index];
		}
	}

	struct PerlinInfos
	{
		siv::PerlinNoise::seed_type seed;
		siv::PerlinNoise perlin;
	};

	struct PerlinManagerData
	{
		std::unordered_map<uint64_t, PerlinInfos> m_PerlinMap;

		std::vector<glm::vec2> curve;
	};

	static PerlinManagerData* s_Data = nullptr;

	void PerlinManager::Init()
	{
		s_Data = new PerlinManagerData();

		s_Data->curve = {
			{ -1.0,80.0 },
			{ -0.8,15.0 },
			{ -0.5,20.0 },
			{ -0.3,50.0 },
			{ -0.2,70.0 },
			{ -0.1,72.0 },
			{ -0.0,75.0 },
			{  0.1,80.0 },
			{  0.2,100.0 },
			{ 0.36,150.0 },
			{  0.5,170.0 },
			{  0.6,190.0 },
			{  1.0,220.0 }
		};
	}

	uint64_t PerlinManager::AddPerlinNoise()
	{
		uint64_t uuid = UUID();
		s_Data->m_PerlinMap[uuid] = { 8173561, siv::PerlinNoise() };
		return uuid;
	}

	float PerlinManager::GetPerlinNoise(uint64_t uuid, float x, float z)
	{
		return s_Data->m_PerlinMap[uuid].perlin.octave2D(x * 0.001f, z * 0.001f, 5);
	}

	float PerlinManager::GetMapHeight(uint64_t uuid, float x, float z)
	{
		float noise_value = GetPerlinNoise(uuid, x, z);

		std::vector<glm::vec2> points(4);

		glm::vec2 leftPoint = Utils::GetLeftPoint(s_Data->curve, noise_value);
		glm::vec2 rightPoint = Utils::GetRightPoint(s_Data->curve, noise_value);

		points[0] = Utils::GetLeftPoint(s_Data->curve, leftPoint.x);
		points[1] = leftPoint;
		points[2] = rightPoint;
		points[3] = Utils::GetRightPoint(s_Data->curve, rightPoint.x);

		float value = Math::MapRange(noise_value - points[1].x, 0.0f, points[2].x - points[1].x, 0.0f, 1.0f);
		float y = Interpolator::CatmullRomInterpolation(value, points[0], points[1], points[2], points[3]).y;

		return y;
	}
}