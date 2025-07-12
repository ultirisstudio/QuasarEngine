#pragma once

#include "BiomeType.h"

class BiomeInfos
{
private:
	float m_HeightWeight;
	float m_DetailScale;
	float m_DetailWeight;
	unsigned m_DetailMaxHeight;
	float m_MountainBias;
public:
	BiomeInfos();
	BiomeInfos(float heightWeight, float detailScale, float detailWeight, unsigned detailMaxHeight, float mountainsBias);
	~BiomeInfos();

	float GetHeightWeight() const;
	float GetDetailScale() const;
	float GetDetailWeight() const;
	unsigned GetDetailMaxHeight() const;
	float GetMountainBias() const;
};
