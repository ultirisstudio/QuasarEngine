#pragma once

#include "Block.h"

#include <glm/glm.hpp>
#include <utility>
#include <array>

class BlockInfos
{
private:
	std::array<float, 6> m_TexCoords;
	bool m_IsTransparent;
public:
	BlockInfos();
	BlockInfos(std::array<float, 6> texCoords, bool isTransparent);

	const std::array<float, 6>& GetTexCoords() const;
	bool IsTransparent() const;
};

typedef std::pair<glm::ivec3, Block> BlockInfo;