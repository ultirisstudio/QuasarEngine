#pragma once

#include <glm/glm.hpp>
#include <QuasarEngine/Tools/Math.h>

namespace Math
{
	float DistToBlock(glm::vec3 pos, QuasarEngine::Math::Axis axis, glm::vec3 dir);
	float DistToBlock(glm::vec3 pos, QuasarEngine::Math::Axis axis, bool negative);
	float DistToBlock(float pos, QuasarEngine::Math::Axis axis, bool negative);
	
	glm::ivec3 ToChunkPosition(const glm::ivec3& position);
	glm::u8vec3 ToBlockPosition(const glm::ivec3& position);
}
