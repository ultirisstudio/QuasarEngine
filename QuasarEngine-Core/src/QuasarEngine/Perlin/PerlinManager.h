#pragma once

#include <unordered_map>

namespace QuasarEngine
{
	class UUID;

	class PerlinManager
	{
	public:
		static void Init();

		static UUID AddPerlinNoise();

		static float GetPerlinNoise(UUID uuid, float x, float z);

		static float GetMapHeight(UUID uuid, float x, float z);
	};
}