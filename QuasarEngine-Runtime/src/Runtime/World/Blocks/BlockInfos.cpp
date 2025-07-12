#include "BlockInfos.h"

BlockInfos::BlockInfos() : m_TexCoords(), m_IsTransparent(false)
{
}

BlockInfos::BlockInfos(std::array<float, 6> texCoords, bool isTransparent) : m_TexCoords(texCoords), m_IsTransparent(isTransparent)
{

}

const std::array<float, 6>& BlockInfos::GetTexCoords() const
{
	return m_TexCoords;
}

bool BlockInfos::IsTransparent() const
{
	return m_IsTransparent;
}
