#pragma once

#include "Chunk.h"
#include "ChunkMapping.h"

#include "../Blocks/Block.h"
#include "../Blocks/BlockInfos.h"

#include "../Biomes/BiomeInfos.h"

#include "../Generation/TerrainGenerator.h"

#include "../../Utils/Math.h"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Core/UUID.h>

class ChunkManager
{
public:
	ChunkManager();
	~ChunkManager() = default;

	static ChunkManager* GetInstance() { return s_Instance; }

	void SetBlock(const glm::ivec3& position, BlockType voxel);
	const Block* GetBlock(const glm::ivec3& position);
	const BlockType GetBlockType(const glm::ivec3& position);

	void AddChunk(const glm::ivec3& position);
	Chunk* GetChunk(const glm::ivec3& position);
	const Chunk* GetChunk(const glm::ivec3& position) const;

	int NeighborCount(glm::ivec3 coord, glm::ivec3 exclude) const;
	bool ChunkInRange(glm::vec3 playerPos, glm::vec3 chunkPos) const;

	void UpdateChunk(const glm::ivec3& playerPos, float dt);
	bool IsTransparent(const glm::ivec3& position);

	BlockInfos& GetBlockInfos(const BlockType& type);
	BiomeInfos& GetBiomeInfos(const BiomeType& type);
private:
	static ChunkManager* s_Instance;

	//typedef std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ChunkMapping, ChunkMapping> ChunkMap;
	typedef std::unordered_map<glm::ivec3, QuasarEngine::UUID, ChunkMapping, ChunkMapping> EntityMap;

	std::unordered_map<BlockType, BlockInfos> m_BlockInfos;
	std::unordered_map<BiomeType, BiomeInfos> m_BiomeInfos;

	//ChunkMap m_ChunkMap;
	EntityMap m_EntityMap;

	std::unique_ptr<TerrainGenerator> m_Generator;
public:
	ChunkManager(ChunkManager const&) = delete;
	void operator=(ChunkManager const&) = delete;
};