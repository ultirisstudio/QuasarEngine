#include "Block.h"

Block::Block() : m_Type(BlockType::AIR)
{
}

Block::Block(BlockType type) : m_Type(type)
{
}

BlockType Block::GetType() const
{
	return m_Type;
}

void Block::SetType(BlockType type)
{
	m_Type = type;
}
