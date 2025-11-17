#pragma once

#include "Chunk.h"
#include "ChunkMapping.h"

#include "../Generation/TerrainGenerator.h"
#include "../../Utils/Math.h"
#include "../Blocks/BlockInfos.h"
#include "../Biomes/BiomeInfos.h"
#include "../../World/Constants.h"

#include <QuasarEngine/Thread/ThreadPool.h>
#include <QuasarEngine/Core/UUID.h>

#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>

struct ChunkBlocksResult
{
	glm::ivec3 position;
	std::array<Block, CHUNK_VOLUME> blocks;
	int maxHeight;
};

struct ChunkMeshResult
{
	glm::ivec3 position;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
};

class ChunkManager
{
public:
    ChunkManager();
    ~ChunkManager() = default;

    static ChunkManager* GetInstance() { return s_Instance; }

    void SetBlock(const glm::ivec3& position, BlockType voxel);
    const Block* GetBlock(const glm::ivec3& position);
    const BlockType GetBlockType(const glm::ivec3& position);

    Chunk* GetChunk(const glm::ivec3& position);
    const Chunk* GetChunk(const glm::ivec3& position) const;

    void UpdateChunks(const glm::ivec3& playerPos, float dt);

    BlockInfos& GetBlockInfos(const BlockType& type);
    BiomeInfos& GetBiomeInfos(const BiomeType& type);
    bool IsTransparent(const glm::ivec3& position);

private:
    static ChunkManager* s_Instance;

    using EntityMap = std::unordered_map<glm::ivec3, QuasarEngine::UUID, ChunkMapping, ChunkMapping>;

    EntityMap m_EntityMap;

    std::unordered_map<BlockType, BlockInfos> m_BlockInfos;
    std::unordered_map<BiomeType, BiomeInfos> m_BiomeInfos;

    std::unique_ptr<TerrainGenerator> m_Generator;

    QuasarEngine::ThreadPool m_GenerationPool;
    QuasarEngine::ThreadPool m_MeshPool;

    std::mutex m_GenResultMutex;
    std::queue<ChunkBlocksResult> m_GenResults;

    std::mutex m_MeshResultMutex;
    std::queue<ChunkMeshResult> m_MeshResults;

    std::mutex m_ChunkMapMutex;

private:
    void RequestChunk(const glm::ivec3& chunkPos);
    void UnloadFarChunks(const glm::ivec3& playerChunkPos);

    void ProcessGenerationResults();
    void ProcessMeshResults();

    void AsyncGenerateBlocks(const glm::ivec3& chunkPos);
    void AsyncGenerateMesh(const glm::ivec3& chunkPos);

    bool ChunkInRange(const glm::ivec3& playerChunkPos, const glm::ivec3& chunkPos) const;

public:
    ChunkManager(ChunkManager const&) = delete;
    void operator=(ChunkManager const&) = delete;
};
