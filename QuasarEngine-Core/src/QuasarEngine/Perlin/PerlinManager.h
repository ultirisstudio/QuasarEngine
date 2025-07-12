#pragma once

#include <unordered_map>

namespace QuasarEngine
{
	class PerlinManager
	{
	public:
		static void Init();

		static uint64_t AddPerlinNoise();

		static float GetPerlinNoise(uint64_t uuid, float x, float z);

		static float GetMapHeight(uint64_t uuid, float x, float z);
	};
}