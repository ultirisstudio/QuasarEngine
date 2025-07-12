#include "Chunk.h"

#include "ChunkManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/compatibility.hpp"
#include <memory>
#include <random>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>

Chunk::Chunk(const glm::ivec3& position) : m_Position(position), m_MaxHeight(0), m_Blocks(), m_HeightTimer(0.0f), m_HeightTimerIncreasing(true)
{

}

Chunk::Chunk(const glm::ivec3& position, BlockType voxel) : m_Position(position), m_MaxHeight(0), m_Blocks(), m_HeightTimer(0.0f), m_HeightTimerIncreasing(true)
{
	m_Blocks.fill(Block(voxel));
}

Chunk::~Chunk()
{

}

void Chunk::SetBlock(const glm::ivec3& pos, Block voxel)
{
	if (ToIndex(pos) < 0 || ToIndex(pos) >= m_Blocks.size())
	{
		//ChunkManager::GetInstance()->SetBlock(pos + m_Position, voxel.GetType());
		return;
	}
	else
	{
		m_Blocks.at(ToIndex(pos)) = voxel;
	}

	if (pos.y > m_MaxHeight)
		m_MaxHeight = pos.y;
}

const Block* Chunk::GetBlock(const glm::ivec3& position) const
{
	if (ToIndex(position) < 0 || ToIndex(position) >= m_Blocks.size())
		return nullptr;

	return &m_Blocks.at(ToIndex(position));
}

const BlockType Chunk::GetBlockType(const glm::ivec3& position) const
{
	if (ToIndex(position) < 0 || ToIndex(position) >= m_Blocks.size())
		return BlockType::BLOCK_ERROR;

	return m_Blocks.at(ToIndex(position)).GetType();
}

void Chunk::Generate(TerrainGenerator& generator)
{
	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		for (int k = 0; k < CHUNK_SIZE; k++)
		{
			glm::ivec2 hpos = glm::ivec2(m_Position.x + i, m_Position.z + k);
			int height = generator.GetHeight(hpos);

			for (int j = 0; j < height; j++)
			{
				Block temp;

				if (j < height - 8)
					temp = Block(BlockType::COBBLE);
				else if (j < height - 1)
					temp = Block(BlockType::DIRT);
				else
					temp = Block(BlockType::GRASS);

				SetBlock(glm::vec3(i, j, k), temp);
			}

			/*if (height < 45)
			{
				for (int j = height; j < 45; j++)
				{
				Block temp = Block(BlockType::STONE);
				SetBlock(glm::vec3(i, j, k), temp);
				}
			}*/
		}
	}

	std::vector<glm::ivec2> points;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> tree(0, CHUNK_SIZE);

	for (int i = 0; i < 8; i++) // 4
	{
		points.push_back(glm::ivec2(tree(gen), tree(gen)));
	}

	for (int i = 0; i < points.size(); i++)
	{
		glm::ivec2 pos = points[i];
		glm::ivec2 hpos = glm::ivec2(m_Position.x, m_Position.z) + points[i];
		int height = generator.GetHeight(hpos);
		generator.GenerateTree(*this, pos.x, height, pos.y);
	}
}

void Chunk::GenerateMesh()
{
	std::vector<float> vertices;

	for (int y = 0; y <= m_MaxHeight; y++)
	{
		for (int z = 0; z < CHUNK_SIZE; z++)
		{
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				BlockType block = m_Blocks.at(ToIndex({ x, y, z })).GetType();

				if (block != BlockType::AIR)
				{
					static std::mt19937 generator(std::random_device{}());
					static std::uniform_int_distribution<int> distribution(0, 10000);

					int seed = distribution(generator);

					std::array<float, 6> texCoords = ChunkManager::GetInstance()->GetBlockInfos(block).GetTexCoords();

					if (x == CHUNK_SIZE - 1 ? ChunkManager::GetInstance()->IsTransparent({ m_Position.x + x + 1, m_Position.y + y, m_Position.z + z }) == true : ChunkManager::GetInstance()->GetBlockInfos(m_Blocks.at(ToIndex({ x + 1, y, z })).GetType()).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							0.5f + x + m_Position.x, -0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,// texCoords.at(3),
							0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,// texCoords.at(3),
							0.5f + x + m_Position.x,  0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,// texCoords.at(3),
							0.5f + x + m_Position.x,  0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f//, texCoords.at(3)
						});
					}

					if (x == 0 ? ChunkManager::GetInstance()->IsTransparent({ m_Position.x + x - 1, m_Position.y + y, m_Position.z + z }) == true : ChunkManager::GetInstance()->GetBlockInfos(m_Blocks.at(ToIndex({ x - 1, y, z })).GetType()).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,// texCoords.at(2),
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y,  0.5f + z + m_Position.z, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,// texCoords.at(2),
							-0.5f + x + m_Position.x,  0.5f + y + m_Position.y,  0.5f + z + m_Position.z, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,// texCoords.at(2),
							-0.5f + x + m_Position.x,  0.5f + y + m_Position.y, -0.5f + z + m_Position.z, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f//, texCoords.at(2)
						});
					}

					if (z == CHUNK_SIZE - 1 ? ChunkManager::GetInstance()->IsTransparent({ m_Position.x + x, m_Position.y + y, m_Position.z + z + 1 }) == true : ChunkManager::GetInstance()->GetBlockInfos(m_Blocks.at(ToIndex({ x, y, z + 1 })).GetType()).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y, 0.5f + z + m_Position.z, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,// texCoords.at(4),
							 0.5f + x + m_Position.x, -0.5f + y + m_Position.y, 0.5f + z + m_Position.z, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,// texCoords.at(4),
							 0.5f + x + m_Position.x,  0.5f + y + m_Position.y, 0.5f + z + m_Position.z, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,// texCoords.at(4),
							-0.5f + x + m_Position.x,  0.5f + y + m_Position.y, 0.5f + z + m_Position.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f//, texCoords.at(4)
						});
					}

					if (z == 0 ? ChunkManager::GetInstance()->IsTransparent({ m_Position.x + x, m_Position.y + y, m_Position.z + z - 1 }) == true : ChunkManager::GetInstance()->GetBlockInfos(m_Blocks.at(ToIndex({ x, y, z - 1 })).GetType()).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							 0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,// texCoords.at(5),
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,// texCoords.at(5),
							-0.5f + x + m_Position.x,  0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,// texCoords.at(5),
							 0.5f + x + m_Position.x,  0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f//, texCoords.at(5)
						});
					}

					if (ChunkManager::GetInstance()->GetBlockInfos(GetBlockType({ x, y - 1, z })).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,// texCoords.at(1),
							 0.5f + x + m_Position.x, -0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,// texCoords.at(1),
							 0.5f + x + m_Position.x, -0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,// texCoords.at(1),
							-0.5f + x + m_Position.x, -0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f//, texCoords.at(1)
						});
					}

					if (ChunkManager::GetInstance()->GetBlockInfos(GetBlockType({ x, y + 1, z })).IsTransparent() == true)
					{
						vertices.insert(vertices.end(), {
							-0.5f + x + m_Position.x, 0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// texCoords.at(0),
							 0.5f + x + m_Position.x, 0.5f + y + m_Position.y,  0.5f + z + m_Position.z, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,// texCoords.at(0),
							 0.5f + x + m_Position.x, 0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,// texCoords.at(0),
							-0.5f + x + m_Position.x, 0.5f + y + m_Position.y, -0.5f + z + m_Position.z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f//, texCoords.at(0)
						});
					}
				}
			}
		}
	}

	std::vector<unsigned int> indices;
	indices.reserve(vertices.size() / 4 * 6);

	for (unsigned int i = 0; i < vertices.size() / 4; i++)
	{
		indices.insert(indices.end(), { i * 4 + 0, i * 4 + 1, i * 4 + 2, i * 4 + 0, i * 4 + 2, i * 4 + 3 });
	}

	if (vertices.size() == 0)
		return;

	if (indices.size() == 0)
		return;

	QuasarEngine::Entity entity{ entt_entity, registry };
	QuasarEngine::BufferLayout layout = {
		{ QuasarEngine::ShaderDataType::Vec3,  "vPosition"				},
		{ QuasarEngine::ShaderDataType::Vec3,  "vNormal"				},
		{ QuasarEngine::ShaderDataType::Vec2,  "vTextureCoordinates"	}//,
		//{ QuasarEngine::ShaderDataType::Float, "vTextureIndice"	}
	};
	entity.GetComponent<QuasarEngine::MeshComponent>().GenerateMesh(vertices, indices, layout);
}

const glm::vec3& Chunk::GetRenderPos() const
{
	float t = 1.0f - m_HeightTimer;

	return glm::vec3(m_Position) - glm::vec3(0.0f, glm::lerp(0.0f, 128.0f, t * t * t), 0.0f);
}

void Chunk::UpdateHeightTimer(float dt)
{
	if (IsMeshGenerated())
	{
		if (m_HeightTimerIncreasing)
			m_HeightTimer += 1.0f * dt;
		else
			m_HeightTimer -= 0.25f * dt;

		m_HeightTimer = glm::clamp(m_HeightTimer, 0.0f, 1.0f);
	}
}

void Chunk::SetHeightTimerIncreasing(bool increasing)
{
	m_HeightTimerIncreasing = increasing;
}

bool Chunk::HeightTimerHitZero() const
{
	return m_HeightTimer == 0.0f && !m_HeightTimerIncreasing;
}

void Chunk::ClearMesh()
{
	QuasarEngine::Entity entity{ entt_entity, registry };
	entity.GetComponent<QuasarEngine::MeshComponent>().ClearMesh();
	//QuasarEngine::Entity entity{ entt_entity, registry };
	//entity.GetComponent<QuasarEngine::MeshComponent>().Clear();
	m_HeightTimer = 0.0f;
}

bool Chunk::IsMeshGenerated() const
{
	//return m_Mesh->Size() != 0;
	QuasarEngine::Entity entity{ entt_entity, registry };
	if (entity.GetComponent<QuasarEngine::MeshComponent>().HasMesh())
		return entity.GetComponent<QuasarEngine::MeshComponent>().GetMesh().IsMeshGenerated();
	else
		return false;
}

int Chunk::ToIndex(const glm::ivec3& position) const
{
	if (position.x < 0 || position.x >= CHUNK_SIZE)
		return -1;

	if (position.y < 0 || position.y >= CHUNK_HEIGHT)
		return -1;

	if (position.z < 0 || position.z >= CHUNK_SIZE)
		return -1;

	return position.y * (CHUNK_AREA)+position.z * (CHUNK_SIZE)+position.x;
}
