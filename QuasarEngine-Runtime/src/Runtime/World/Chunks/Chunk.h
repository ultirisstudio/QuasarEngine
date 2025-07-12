#pragma once

#include "../Constants.h"
#include "../../World/Blocks/Block.h"
#include "../../World/Blocks/BlockType.h"
#include "../../Utils/Math.h"

#include <array>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Component.h>

class ChunkManager;
class TerrainGenerator;

class Chunk : public QuasarEngine::Component
{
public:
	Chunk(const glm::ivec3& position);
	Chunk(const glm::ivec3& position, BlockType voxel);
	~Chunk();

	void SetBlock(const glm::ivec3& pos, Block voxel);
	const Block* GetBlock(const glm::ivec3& position) const;
	const BlockType GetBlockType(const glm::ivec3& position) const;

	void Generate(TerrainGenerator& generator);
	void GenerateMesh();
	void ClearMesh();
	bool IsMeshGenerated() const;

	//BlockType GetBlockFromGeneration(const glm::ivec3& position) const;

	int GetMaxHeight() const { return m_MaxHeight; }

	const glm::ivec3& GetPosition() { return m_Position; }
	const glm::vec3& GetRenderPos() const;

	void UpdateHeightTimer(float dt);
	void SetHeightTimerIncreasing(bool increasing);
	bool HeightTimerHitZero() const;

private:
	std::array<Block, CHUNK_VOLUME> m_Blocks;

	const glm::ivec3 m_Position;
	int m_MaxHeight;
	float m_HeightTimer;
	bool m_HeightTimerIncreasing;

	int ToIndex(const glm::ivec3& position) const;
};