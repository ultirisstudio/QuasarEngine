#pragma once

#include <glm/glm.hpp>

struct ChunkMapping
{
	size_t operator()(const glm::ivec3& k)const
	{
		return std::hash<int>()(k.x) ^ std::hash<int>()(k.y) ^ std::hash<int>()(k.z);
	}

	bool operator()(const glm::ivec3& a, const glm::ivec3& b)const
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
};