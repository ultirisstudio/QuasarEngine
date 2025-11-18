#include "ChunkManager.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>
#include <QuasarEngine/Resources/TextureArray.h>

ChunkManager* ChunkManager::s_Instance = nullptr;

ChunkManager::ChunkManager()
	: m_GenerationPool(std::max(1u, std::thread::hardware_concurrency() / 2))
	, m_MeshPool(std::max(1u, std::thread::hardware_concurrency() / 2))
{
	s_Instance = this;

	m_Generator = std::make_unique<TerrainGenerator>();
	//m_Shader = std::make_unique<QuasarEngine::Shader>("Shaders/basic_vertex.glsl", "Shaders/basic_fragment.glsl");

	std::vector<std::string> textures {
		"Assets/Textures/dark_grass_block_top.png",		//1
		"Assets/Textures/dark_grass_block_side.png",	//2
		"Assets/Textures/dark_dirt.png",				//3
		"Assets/Textures/dark_cobblestone.png",			//4
		"Assets/Textures/stone.png",					//5
		"Assets/Textures/log_1.png",					//6
		"Assets/Textures/log_top.png",					//7
		"Assets/Textures/leaves_test.png",				//8
		"Assets/Textures/library_1.png",				//9
		"Assets/Textures/library_2.png",				//10
	};

	std::vector<std::string> normal_textures {
		"Assets/Textures/dark_grass_block_top_normal.png",
		"Assets/Textures/dark_grass_block_side_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
		"Assets/Textures/dark_dirt_normal.png",
	};

	// UP, DOWN, LEFT, RIGHT, FRONT, BACK
	m_BlockInfos[BlockType::BLOCK_ERROR] = BlockInfos({ 0, 0, 0, 0, 0, 0 }, true);
	m_BlockInfos[BlockType::AIR] = BlockInfos({ 0, 0, 0, 0, 0, 0 }, true);
	m_BlockInfos[BlockType::GRASS] = BlockInfos({ 1, 1, 2, 2, 2, 2 }, false);
	m_BlockInfos[BlockType::DIRT] = BlockInfos({ 3, 3, 3, 3, 3, 3 }, false);
	m_BlockInfos[BlockType::COBBLE] = BlockInfos({ 4, 4, 4, 4, 4, 4 }, false);
	m_BlockInfos[BlockType::STONE] = BlockInfos({ 5, 5, 5, 5, 5, 5 }, false);
	m_BlockInfos[BlockType::LOG] = BlockInfos({ 7, 7, 6, 6, 6, 6 }, false);
	m_BlockInfos[BlockType::LEAVES] = BlockInfos({ 8, 8, 8, 8, 8, 8 }, true);
	m_BlockInfos[BlockType::LIBRARY_1] = BlockInfos({ 6, 6, 6, 6, 9, 9 }, true);
	m_BlockInfos[BlockType::LIBRARY_2] = BlockInfos({ 6, 6, 6, 6, 10, 10 }, true);


	m_BiomeInfos[BiomeType::MONTAINS] = BiomeInfos(0.8f, 32.0f, 1.0f, 100, -0.2f);
	m_BiomeInfos[BiomeType::PLAINS] = BiomeInfos(0.8f, 32.0f, 1.0f, 100, -0.6f);
	m_BiomeInfos[BiomeType::FOREST] = BiomeInfos(0.6f, 32.0f, 0.8f, 100, 0.0f);
	m_BiomeInfos[BiomeType::DESERT] = BiomeInfos(0.1f, 8.0f, 0.1f, 100, 0.0f);
	m_BiomeInfos[BiomeType::SNOW] = BiomeInfos(0.1f, 8.0f, 0.1f, 100, 0.0f);

	//std::unique_ptr<QuasarEngine::TextureArray> textures = std::make_unique<QuasarEngine::TextureArray>(textures);

	QuasarEngine::TextureSpecification spec;
	spec.gamma = true;
	spec.alpha = true;
	std::shared_ptr<QuasarEngine::TextureArray> textureArray = QuasarEngine::TextureArray::Create(spec);
	textureArray->LoadFromFiles(textures);

	QuasarEngine::TextureSpecification n_spec;
	n_spec.alpha = true;
	std::shared_ptr<QuasarEngine::TextureArray> normalTextureArray = QuasarEngine::TextureArray::Create(n_spec);
	normalTextureArray->LoadFromFiles(normal_textures);

	QuasarEngine::AssetManager::Instance().instLoadAsset("textures", textureArray);
	QuasarEngine::AssetManager::Instance().instLoadAsset("normal_textures", normalTextureArray);
}

void ChunkManager::SetBlock(const glm::ivec3& position, BlockType voxel)
{
	auto chunkPos = Math::ToChunkPosition(position);
	auto it = m_EntityMap.find(chunkPos);
	if (it == m_EntityMap.end())
		return;

	auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
	if (!entityOpt.has_value())
		return;

	auto& entity = entityOpt.value();
	auto& chunk = entity.GetComponent<Chunk>();
	if (!chunk.IsMeshGenerated())
		return;

	chunk.SetBlock(glm::u8vec3(Math::ToBlockPosition(position)), Block(voxel));
	chunk.GenerateMesh();
	chunk.ClearMesh();

	auto updateNeighborChunk = [&](const glm::ivec3& neighborPos) {
		auto neighborIt = m_EntityMap.find(Math::ToChunkPosition(neighborPos));
		if (neighborIt == m_EntityMap.end())
			return;
		auto neighborOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(neighborIt->second);
		if (!neighborOpt.has_value())
			return;
		auto& neighborEntity = neighborOpt.value();
		auto& neighborChunk = neighborEntity.GetComponent<Chunk>();
		neighborChunk.GenerateMesh();
		neighborChunk.ClearMesh();
		};

	auto blockPos = Math::ToBlockPosition(position);

	if (blockPos.z == 0)
		updateNeighborChunk(position + glm::ivec3(0, 0, -CHUNK_SIZE));
	else if (blockPos.z == CHUNK_SIZE - 1)
		updateNeighborChunk(position + glm::ivec3(0, 0, CHUNK_SIZE));

	if (blockPos.x == 0)
		updateNeighborChunk(position + glm::ivec3(-CHUNK_SIZE, 0, 0));
	else if (blockPos.x == CHUNK_SIZE - 1)
		updateNeighborChunk(position + glm::ivec3(CHUNK_SIZE, 0, 0));
}

const Block* ChunkManager::GetBlock(const glm::ivec3& position)
{
	auto chunkPos = Math::ToChunkPosition(position);
	auto it = m_EntityMap.find(chunkPos);
	if (it == m_EntityMap.end())
		return nullptr;

	auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
	if (!entityOpt.has_value())
		return nullptr;

	auto& entity = entityOpt.value();
	auto& chunk = entity.GetComponent<Chunk>();
	return chunk.GetBlock(Math::ToBlockPosition(position));
}

const BlockType ChunkManager::GetBlockType(const glm::ivec3& position)
{
	auto chunkPos = Math::ToChunkPosition(position);
	auto it = m_EntityMap.find(chunkPos);
	if (it == m_EntityMap.end())
		return BlockType::BLOCK_ERROR;

	auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
	if (!entityOpt.has_value())
		return BlockType::BLOCK_ERROR;

	auto& entity = entityOpt.value();
	auto& chunk = entity.GetComponent<Chunk>();
	return chunk.GetBlockType(Math::ToBlockPosition(position));
}

Chunk* ChunkManager::GetChunk(const glm::ivec3& position)
{
	return const_cast<Chunk*>(static_cast<const ChunkManager&>(*this).GetChunk(position));
}

const Chunk* ChunkManager::GetChunk(const glm::ivec3& position) const
{
	auto it = m_EntityMap.find(position);
	if (it == m_EntityMap.end())
		return nullptr;

	auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
	if (!entityOpt.has_value())
		return nullptr;

	return &entityOpt.value().GetComponent<Chunk>();
}

void ChunkManager::UpdateChunks(const glm::ivec3& playerPos, float dt)
{
	ProcessGenerationResults();
	ProcessMeshResults();

	glm::ivec3 playerChunkPos = Math::ToChunkPosition(playerPos);

	for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; ++dz)
	{
		for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; ++dx)
		{
			glm::ivec3 cpos = playerChunkPos + glm::ivec3(dx * CHUNK_SIZE, 0, dz * CHUNK_SIZE);

			if (!ChunkInRange(playerChunkPos, cpos))
				continue;

			RequestChunk(cpos);
		}
	}

	UnloadFarChunks(playerChunkPos);
}

void ChunkManager::RequestChunk(const glm::ivec3& chunkPos)
{
	std::lock_guard<std::mutex> lock(m_ChunkMapMutex);

	if (m_EntityMap.find(chunkPos) != m_EntityMap.end())
		return;

	std::string name = "Chunk_" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + "_" + std::to_string(chunkPos.z);
	QuasarEngine::Entity entity = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->CreateEntity(name);

	entity.AddComponent<QuasarEngine::MeshComponent>();
	QuasarEngine::MaterialSpecification spec;
	spec.AlbedoTexture = "Assets/Textures/dark_grass_block_top.png";
	spec.Metallic = 0.0f;
	spec.Roughness = 1.0f;
	entity.AddComponent<QuasarEngine::MaterialComponent>(spec);
	entity.AddComponent<QuasarEngine::MeshRendererComponent>();
	entity.AddComponent<Chunk>(chunkPos);

	m_EntityMap.emplace(chunkPos, entity.GetUUID());

	m_GenerationPool.Enqueue([this, chunkPos]() {
		AsyncGenerateBlocks(chunkPos);
		});
}

void ChunkManager::AsyncGenerateBlocks(const glm::ivec3& chunkPos)
{
	ChunkBlocksResult result;
	result.position = chunkPos;
	result.maxHeight = 0;
	result.blocks.fill(Block(BlockType::AIR));

	int heightmap[CHUNK_SIZE][CHUNK_SIZE];
	m_Generator->GenerateHeightmap(chunkPos, heightmap);

	for (int x = 0; x < CHUNK_SIZE; ++x)
	{
		for (int z = 0; z < CHUNK_SIZE; ++z)
		{
			int height = heightmap[x][z];

			if (height > result.maxHeight)
				result.maxHeight = height - 1;

			for (int y = 0; y < height; ++y)
			{
				BlockType type;
				if (y < height - 8)      type = BlockType::COBBLE;
				else if (y < height - 1) type = BlockType::DIRT;
				else                     type = BlockType::GRASS;

				int idx = y * CHUNK_AREA + z * CHUNK_SIZE + x;
				result.blocks[idx] = Block(type);
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_GenResultMutex);
		m_GenResults.push(std::move(result));
	}
}

void ChunkManager::ProcessGenerationResults()
{
	std::queue<ChunkBlocksResult> localQueue;

	{
		std::lock_guard<std::mutex> lock(m_GenResultMutex);
		std::swap(localQueue, m_GenResults);
	}

	while (!localQueue.empty())
	{
		ChunkBlocksResult res = std::move(localQueue.front());
		localQueue.pop();

		auto it = m_EntityMap.find(res.position);
		if (it == m_EntityMap.end())
			continue;

		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
		if (!entityOpt.has_value())
			continue;

		auto& entity = entityOpt.value();
		auto& chunk = entity.GetComponent<Chunk>();

		chunk.SetBlocksFromExternal(res.blocks, res.maxHeight);

		m_MeshPool.Enqueue([this, pos = res.position]() {
			AsyncGenerateMesh(pos);
			});

		std::array<glm::ivec3, 4> neighbors = {
			res.position + glm::ivec3(CHUNK_SIZE, 0,          0),
			res.position + glm::ivec3(-CHUNK_SIZE, 0,          0),
			res.position + glm::ivec3(0,          0,  CHUNK_SIZE),
			res.position + glm::ivec3(0,          0, -CHUNK_SIZE)
		};

		for (const auto& nPos : neighbors)
		{
			std::lock_guard<std::mutex> lock(m_ChunkMapMutex);
			auto nit = m_EntityMap.find(nPos);
			if (nit == m_EntityMap.end())
				continue;

			m_MeshPool.Enqueue([this, pos = nPos]() {
				AsyncGenerateMesh(pos);
				});
		}
	}
}

void ChunkManager::AsyncGenerateMesh(const glm::ivec3& chunkPos)
{
	ChunkMeshResult result;
	result.position = chunkPos;

	Chunk* chunk = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_ChunkMapMutex);
		auto it = m_EntityMap.find(chunkPos);
		if (it == m_EntityMap.end())
			return;

		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
		if (!entityOpt.has_value())
			return;

		chunk = &entityOpt.value().GetComponent<Chunk>();
	}

	chunk->BuildGreedyMeshData(result.vertices, result.indices);

	{
		std::lock_guard<std::mutex> lock(m_MeshResultMutex);
		m_MeshResults.push(std::move(result));
	}
}

void ChunkManager::ProcessMeshResults()
{
	std::queue<ChunkMeshResult> localQueue;
	{
		std::lock_guard<std::mutex> lock(m_MeshResultMutex);
		std::swap(localQueue, m_MeshResults);
	}

	while (!localQueue.empty())
	{
		ChunkMeshResult res = std::move(localQueue.front());
		localQueue.pop();

		auto it = m_EntityMap.find(res.position);
		if (it == m_EntityMap.end())
			continue;

		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
		if (!entityOpt.has_value())
			continue;

		auto& entity = entityOpt.value();
		auto& meshComp = entity.GetComponent<QuasarEngine::MeshComponent>();

		if (res.vertices.empty() || res.indices.empty())
		{
			meshComp.ClearMesh();
			continue;
		}

		QuasarEngine::BufferLayout layout = {
			{ QuasarEngine::ShaderDataType::Vec3,  "vPosition" },
			{ QuasarEngine::ShaderDataType::Vec3,  "vNormal" },
			{ QuasarEngine::ShaderDataType::Vec2,  "vTextureCoordinates" }
		};

		meshComp.GenerateMesh(res.vertices, res.indices, layout);
	}
}

void ChunkManager::UnloadFarChunks(const glm::ivec3& playerChunkPos)
{
	std::lock_guard<std::mutex> lock(m_ChunkMapMutex);

	for (auto it = m_EntityMap.begin(); it != m_EntityMap.end();)
	{
		if (!ChunkInRange(playerChunkPos, it->first))
		{
			auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
			if (entityOpt.has_value())
			{
				QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->DestroyEntity(entityOpt.value().GetUUID());
			}
			it = m_EntityMap.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool ChunkManager::ChunkInRange(const glm::ivec3& playerChunkPos, const glm::ivec3& chunkPos) const
{
	glm::ivec3 delta = (chunkPos - playerChunkPos) / CHUNK_SIZE;
	int dx = std::abs(delta.x);
	int dz = std::abs(delta.z);
	return dx <= RENDER_DISTANCE && dz <= RENDER_DISTANCE;
}

BlockInfos& ChunkManager::GetBlockInfos(const BlockType& type)
{
	return m_BlockInfos[type];
}

BiomeInfos& ChunkManager::GetBiomeInfos(const BiomeType& type)
{
	return m_BiomeInfos[type];
}

bool ChunkManager::IsTransparent(const glm::ivec3& position)
{
	return m_BlockInfos[GetBlockType(position)].IsTransparent();
}