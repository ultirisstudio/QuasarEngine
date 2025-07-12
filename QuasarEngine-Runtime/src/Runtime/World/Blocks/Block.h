#pragma once

#include "BlockType.h"
#include <glm/glm.hpp>

class Block
{
private:
	BlockType m_Type;
public:
	Block();
	Block(BlockType type);
	~Block() = default;

	BlockType GetType() const;
	void SetType(BlockType type);
};