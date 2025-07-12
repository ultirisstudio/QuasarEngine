#include "Math.h"
#include "../World/Constants.h"

namespace Math
{
	float Math::DistToBlock(glm::vec3 pos, QuasarEngine::Math::Axis axis, glm::vec3 dir)
	{
		if (dir[axis] == 0.0f)
			return INFINITY;
		else
			return DistToBlock(pos, axis, dir[axis] < 0.0f);
	}

	float Math::DistToBlock(glm::vec3 pos, QuasarEngine::Math::Axis axis, bool negative)
	{
		return DistToBlock(pos[axis], axis, negative);
	}

	float Math::DistToBlock(float pos, QuasarEngine::Math::Axis axis, bool negative)
	{
		float result;

		if (!negative)
			result = 1.0f - glm::fract(pos);
		else
			result = glm::fract(pos);
		if (result == 0.0f)
			result = 1.0f;

		return result;
	}

	glm::ivec3 Math::ToChunkPosition(const glm::ivec3& position)
	{
		int x = position.x < 0 ? (int)-(std::ceil(std::abs((float)position.x / CHUNK_SIZE))) * CHUNK_SIZE : (position.x / CHUNK_SIZE) * CHUNK_SIZE;
		int y = 0;
		int z = position.z < 0 ? (int)-(std::ceil(std::abs((float)position.z / CHUNK_SIZE))) * CHUNK_SIZE : (position.z / CHUNK_SIZE) * CHUNK_SIZE;

		glm::ivec3 result = { x, y, z };

		return result;
	}

	glm::u8vec3 Math::ToBlockPosition(const glm::ivec3& position)
	{
		return {
			position.x < 0 ? (int)((std::ceil(std::abs((float)position.x / CHUNK_SIZE)) * CHUNK_SIZE)) + position.x : (int)position.x % CHUNK_SIZE,
			position.y < 0 ? (int)((std::ceil(std::abs((float)position.y / CHUNK_HEIGHT)) * CHUNK_HEIGHT)) + position.y : (int)position.y % CHUNK_HEIGHT,
			position.z < 0 ? (int)((std::ceil(std::abs((float)position.z / CHUNK_SIZE)) * CHUNK_SIZE)) + position.z : (int)position.z % CHUNK_SIZE
		};
	}
}