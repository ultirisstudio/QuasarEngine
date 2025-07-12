#include "BiomeInfos.h"

BiomeInfos::BiomeInfos() : m_HeightWeight(0), m_DetailScale(0), m_DetailWeight(0), m_MountainBias(0), m_DetailMaxHeight(0)
{
}

BiomeInfos::BiomeInfos(float heightWeight, float detailScale, float detailWeight, unsigned detailMaxHeight, float mountainsBias) : m_HeightWeight(heightWeight), m_DetailScale(detailScale), m_DetailWeight(detailWeight - heightWeight), m_MountainBias(mountainsBias), m_DetailMaxHeight(detailMaxHeight)
{
}

BiomeInfos::~BiomeInfos()
{
}

float BiomeInfos::GetHeightWeight() const
{
	return m_HeightWeight;
}

float BiomeInfos::GetDetailScale() const
{
	return m_DetailScale;
}

float BiomeInfos::GetDetailWeight() const
{
	return m_DetailWeight;
}

unsigned BiomeInfos::GetDetailMaxHeight() const
{
	return m_DetailMaxHeight;
}

float BiomeInfos::GetMountainBias() const
{
	return m_MountainBias;
}
