#include "ChunkManager.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>
#include <QuasarEngine/Resources/TextureArray.h>

ChunkManager* ChunkManager::s_Instance = nullptr;

ChunkManager::ChunkManager()
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

void ChunkManager::AddChunk(const glm::ivec3& position)
{
	if (m_EntityMap.find(position) == m_EntityMap.end())
	{
		std::string name = "Chunk_" + std::to_string(position.x) + "_" + std::to_string(position.y) + "_" + std::to_string(position.z);
		QuasarEngine::Entity chunk = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->CreateEntity(name);
		QuasarEngine::MaterialSpecification spec;
		spec.AlbedoTexture = "Assets/Textures/dark_grass_block_top.png";
		chunk.AddComponent<QuasarEngine::MeshComponent>();
		chunk.AddComponent<QuasarEngine::MaterialComponent>(spec);
		chunk.AddComponent<QuasarEngine::MeshRendererComponent>();
		chunk.AddComponent<Chunk>(position);

		m_EntityMap.emplace(position, chunk.GetUUID());
		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(chunk.GetUUID());
		if (entityOpt.has_value())
			entityOpt.value().GetComponent<Chunk>().Generate(*m_Generator);
	}
	else
	{
		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(m_EntityMap[position]);
		if (entityOpt.has_value() && entityOpt.value().GetComponent<Chunk>().IsMeshGenerated())
			return;
	}

	for (unsigned i = 0; i < std::size(QuasarEngine::Math::surrounding); i++)
	{
		glm::ivec3 newCoord = position + glm::ivec3(QuasarEngine::Math::surrounding[i].x * CHUNK_SIZE, 0, QuasarEngine::Math::surrounding[i].y * CHUNK_SIZE);
		if (GetChunk(newCoord) == nullptr)
		{
			std::string name = "Chunk_" + std::to_string(newCoord.x) + "_" + std::to_string(newCoord.y) + "_" + std::to_string(newCoord.z);
			QuasarEngine::Entity chunk = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->CreateEntity(name);
			chunk.AddComponent<QuasarEngine::MeshComponent>();
			QuasarEngine::MaterialSpecification spec;
			spec.AlbedoTexture = "Assets/Textures/dark_grass_block_top.png";
			chunk.AddComponent<QuasarEngine::MaterialComponent>(spec);
			chunk.AddComponent<QuasarEngine::MeshRendererComponent>();
			chunk.AddComponent<Chunk>(newCoord);

			m_EntityMap.emplace(newCoord, chunk.GetUUID());
			auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(chunk.GetUUID());
			if (entityOpt.has_value())
				entityOpt.value().GetComponent<Chunk>().Generate(*m_Generator);
		}
	}

	auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(m_EntityMap[position]);
	if (entityOpt.has_value())
	{
		auto& chunk = entityOpt.value().GetComponent<Chunk>();
		chunk.GenerateMesh();
		chunk.ClearMesh();
	}
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

int ChunkManager::NeighborCount(glm::ivec3 coord, glm::ivec3 exclude) const
{
	unsigned localCount = 0;
	auto it = m_EntityMap.begin();

	while (localCount < 4 && it != m_EntityMap.end())
	{
		const glm::ivec3& currentCoord = it->first;
		if (currentCoord == exclude)
		{
			++it;
			continue;
		}

		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
		if (entityOpt.has_value())
		{
			auto& chunk = entityOpt.value().GetComponent<Chunk>();
			if (chunk.IsMeshGenerated())
				++localCount;
		}
		++it;
	}
	return localCount;
}

void ChunkManager::UpdateChunk(const glm::ivec3& playerPos, float dt)
{
	unsigned loadedChunks = 0;

	glm::ivec3 playerChunkCoord = Math::ToChunkPosition(glm::floor(glm::vec3(playerPos)));
	Chunk* playerChunk = GetChunk(playerChunkCoord);
	bool isPlayerChunkNull = (playerChunk == nullptr);
	bool isPlayerChunkMeshGenerated = (!isPlayerChunkNull && playerChunk->IsMeshGenerated());

	if (isPlayerChunkNull || !isPlayerChunkMeshGenerated)
	{
		loadedChunks++;
		AddChunk(playerChunkCoord);

		for (int i = 0; i < QuasarEngine::Math::DIRECTION_COUNT; i++)
		{
			if (i == QuasarEngine::Math::DIRECTION_UP || i == QuasarEngine::Math::DIRECTION_DOWN)
				continue;

			loadedChunks++;
			AddChunk(playerChunkCoord + glm::ivec3(QuasarEngine::Math::directionVectors[i].x * CHUNK_SIZE, 0, QuasarEngine::Math::directionVectors[i].z * CHUNK_SIZE));
		}
	}

	for (auto it = m_EntityMap.begin(); it != m_EntityMap.end();)
	{
		auto entityOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(it->second);
		if (!entityOpt.has_value())
		{
			++it;
			continue;
		}

		auto& chunk = entityOpt.value().GetComponent<Chunk>();
		chunk.UpdateHeightTimer(dt);

		bool isCurrentChunkMeshGenerated = chunk.IsMeshGenerated();

		if (ChunkInRange(playerPos, chunk.GetPosition()))
		{
			if (loadedChunks < NUMBER_OF_CHUNKS_TO_GENERATE && !isCurrentChunkMeshGenerated && NeighborCount(it->first, it->first) >= 3)
			{
				loadedChunks++;
				AddChunk(it->first);
			}
			chunk.SetHeightTimerIncreasing(true);
		}
		else if (isCurrentChunkMeshGenerated)
		{
			chunk.SetHeightTimerIncreasing(false);
		}

		if (chunk.HeightTimerHitZero())
		{
			for (unsigned i = 0; i < std::size(QuasarEngine::Math::surrounding); i++)
			{
				glm::ivec3 newCoord = it->first + glm::ivec3(QuasarEngine::Math::surrounding[i].x * CHUNK_SIZE, 0, QuasarEngine::Math::surrounding[i].y * CHUNK_SIZE);
				auto chunkIt = m_EntityMap.find(newCoord);

				if (chunkIt != m_EntityMap.end())
				{
					auto neighborOpt = QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(chunkIt->second);
					if (neighborOpt.has_value() && !neighborOpt.value().GetComponent<Chunk>().IsMeshGenerated() && NeighborCount(newCoord, it->first) == 0)
					{
						QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->DestroyEntity(neighborOpt.value().GetUUID());
						m_EntityMap.erase(chunkIt);
					}
				}
			}

			auto prev = it++;
			QuasarEngine::Renderer::Instance().m_SceneData.m_Scene->DestroyEntity(entityOpt.value().GetUUID());
			m_EntityMap.erase(prev);
		}
		else
		{
			++it;
		}
	}
}

bool ChunkManager::ChunkInRange(glm::vec3 playerPos, glm::vec3 chunkPos) const
{
	glm::vec3 pos = chunkPos + (glm::vec3(CHUNK_SIZE, 0.0f, CHUNK_SIZE) / 2.f);
	float distanceSquared = glm::distance(glm::vec2(pos.x, pos.z), glm::vec2(playerPos.x, playerPos.z));
	return distanceSquared <= RENDER_DISTANCE * CHUNK_SIZE;
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