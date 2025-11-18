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
    int outHeight[CHUNK_SIZE][CHUNK_SIZE];
	generator.GenerateHeightmap(m_Position, outHeight);

	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		for (int k = 0; k < CHUNK_SIZE; k++)
		{
			int height = outHeight[i][k];

			for (int j = 0; j < height; j++)
			{
				Block temp;

				if (j < height - 8)
					temp = Block(BlockType::COBBLE);
				else if (j < height - 1)
					temp = Block(BlockType::DIRT);
				else
					temp = Block(BlockType::GRASS);

				SetBlock(glm::ivec3(i, j, k), temp);
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
	std::vector<unsigned int> indices;

	BuildGreedyMeshData(vertices, indices);

	if (vertices.empty() || indices.empty())
	{
		ClearMesh();
		return;
	}

	QuasarEngine::Entity entity{ entt_entity, registry };
	auto& meshComp = entity.GetComponent<QuasarEngine::MeshComponent>();

	QuasarEngine::BufferLayout layout = {
		{ QuasarEngine::ShaderDataType::Vec3,  "vPosition" },
		{ QuasarEngine::ShaderDataType::Vec3,  "vNormal"   },
		{ QuasarEngine::ShaderDataType::Vec2,  "vTextureCoordinates" }
	};

	meshComp.GenerateMesh(vertices, indices, layout);
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

void Chunk::SetBlocksFromExternal(const std::array<Block, CHUNK_VOLUME>& blocks, int maxHeight)
{
	m_Blocks = blocks;
	m_MaxHeight = maxHeight;
}

void Chunk::BuildGreedyMeshData(std::vector<float>& vertices, std::vector<unsigned int>& indices) const
{
    vertices.clear();
    indices.clear();

    unsigned int indexOffset = 0;

    auto pushQuad = [&](const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        const glm::vec3& v3,
        const glm::vec3& normal)
        {
            vertices.push_back(v0.x); vertices.push_back(v0.y); vertices.push_back(v0.z);
            vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
            vertices.push_back(0.0f); vertices.push_back(0.0f);

            vertices.push_back(v1.x); vertices.push_back(v1.y); vertices.push_back(v1.z);
            vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
            vertices.push_back(1.0f); vertices.push_back(0.0f);

            vertices.push_back(v2.x); vertices.push_back(v2.y); vertices.push_back(v2.z);
            vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
            vertices.push_back(1.0f); vertices.push_back(1.0f);

            vertices.push_back(v3.x); vertices.push_back(v3.y); vertices.push_back(v3.z);
            vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
            vertices.push_back(0.0f); vertices.push_back(1.0f);

            indices.push_back(indexOffset + 0);
            indices.push_back(indexOffset + 1);
            indices.push_back(indexOffset + 2);
            indices.push_back(indexOffset + 0);
            indices.push_back(indexOffset + 2);
            indices.push_back(indexOffset + 3);

            indexOffset += 4;
        };

    const int S = CHUNK_SIZE;
    const int H = CHUNK_HEIGHT;

    std::vector<BlockType> mask(S * S);

    for (int y = 0; y < H; ++y)
    {
        for (int z = 0; z < S; ++z)
        {
            for (int x = 0; x < S; ++x)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x, y + 1, z);

                mask[z * S + x] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int z = 0;
        while (z < S)
        {
            int x = 0;
            while (x < S)
            {
                BlockType t = mask[z * S + x];
                if (t == BlockType::AIR) { x++; continue; }

                int w = 1;
                while (x + w < S && mask[z * S + (x + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (z + h < S && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(z + h) * S + (x + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dz = 0; dz < h; ++dz)
                    for (int dx = 0; dx < w; ++dx)
                        mask[(z + dz) * S + (x + dx)] = BlockType::AIR;

                float x0 = m_Position.x + x - 0.5f;
                float x1 = m_Position.x + x + w - 0.5f;
                float z0 = m_Position.z + z - 0.5f;
                float z1 = m_Position.z + z + h - 0.5f;
                float yy = m_Position.y + y + 0.5f;

                glm::vec3 n(0.0f, 1.0f, 0.0f);

                glm::vec3 v0(x0, yy, z1);
                glm::vec3 v1(x1, yy, z1);
                glm::vec3 v2(x1, yy, z0);
                glm::vec3 v3(x0, yy, z0);

                pushQuad(v0, v1, v2, v3, n);

                x += w;
            }
            z++;
        }
    }

    for (int y = 0; y < H; ++y)
    {
        for (int z = 0; z < S; ++z)
        {
            for (int x = 0; x < S; ++x)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x, y - 1, z);

                mask[z * S + x] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int z = 0;
        while (z < S)
        {
            int x = 0;
            while (x < S)
            {
                BlockType t = mask[z * S + x];
                if (t == BlockType::AIR) { x++; continue; }

                int w = 1;
                while (x + w < S && mask[z * S + (x + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (z + h < S && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(z + h) * S + (x + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dz = 0; dz < h; ++dz)
                    for (int dx = 0; dx < w; ++dx)
                        mask[(z + dz) * S + (x + dx)] = BlockType::AIR;

                float x0 = m_Position.x + x - 0.5f;
                float x1 = m_Position.x + x + w - 0.5f;
                float z0 = m_Position.z + z - 0.5f;
                float z1 = m_Position.z + z + h - 0.5f;
                float yy = m_Position.y + y - 0.5f;

                glm::vec3 n(0.0f, -1.0f, 0.0f);

                glm::vec3 v0(x0, yy, z0);
                glm::vec3 v1(x1, yy, z0);
                glm::vec3 v2(x1, yy, z1);
                glm::vec3 v3(x0, yy, z1);

                pushQuad(v0, v1, v2, v3, n);

                x += w;
            }
            z++;
        }
    }

    mask.resize(H * S);

    for (int x = 0; x < S; ++x)
    {
        for (int y = 0; y < H; ++y)
        {
            for (int z = 0; z < S; ++z)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x + 1, y, z);

                mask[y * S + z] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int y = 0;
        while (y < H)
        {
            int z = 0;
            while (z < S)
            {
                BlockType t = mask[y * S + z];
                if (t == BlockType::AIR) { z++; continue; }

                int w = 1;
                while (z + w < S && mask[y * S + (z + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (y + h < H && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(y + h) * S + (z + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dy = 0; dy < h; ++dy)
                    for (int dz = 0; dz < w; ++dz)
                        mask[(y + dy) * S + (z + dz)] = BlockType::AIR;

                float xx = m_Position.x + x + 0.5f;
                float y0 = m_Position.y + y - 0.5f;
                float y1 = m_Position.y + y + h - 0.5f;
                float z0 = m_Position.z + z - 0.5f;
                float z1 = m_Position.z + z + w - 0.5f;

                glm::vec3 n(1.0f, 0.0f, 0.0f);

                glm::vec3 v0(xx, y0, z1);
                glm::vec3 v1(xx, y0, z0);
                glm::vec3 v2(xx, y1, z0);
                glm::vec3 v3(xx, y1, z1);

                pushQuad(v0, v1, v2, v3, n);

                z += w;
            }
            y++;
        }
    }

    for (int x = 0; x < S; ++x)
    {
        for (int y = 0; y < H; ++y)
        {
            for (int z = 0; z < S; ++z)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x - 1, y, z);

                mask[y * S + z] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int y = 0;
        while (y < H)
        {
            int z = 0;
            while (z < S)
            {
                BlockType t = mask[y * S + z];
                if (t == BlockType::AIR) { z++; continue; }

                int w = 1;
                while (z + w < S && mask[y * S + (z + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (y + h < H && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(y + h) * S + (z + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dy = 0; dy < h; ++dy)
                    for (int dz = 0; dz < w; ++dz)
                        mask[(y + dy) * S + (z + dz)] = BlockType::AIR;

                float xx = m_Position.x + x - 0.5f;
                float y0 = m_Position.y + y - 0.5f;
                float y1 = m_Position.y + y + h - 0.5f;
                float z0 = m_Position.z + z + w - 0.5f;
                float z1 = m_Position.z + z - 0.5f;

                glm::vec3 n(-1.0f, 0.0f, 0.0f);

                glm::vec3 v0(xx, y0, z0);
                glm::vec3 v1(xx, y1, z0);
                glm::vec3 v2(xx, y1, z1);
                glm::vec3 v3(xx, y0, z1);

                pushQuad(v0, v1, v2, v3, n);

                z += w;
            }
            y++;
        }
    }

    mask.resize(H * S);

    for (int z = 0; z < S; ++z)
    {
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < S; ++x)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x, y, z + 1);

                mask[y * S + x] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int y = 0;
        while (y < H)
        {
            int x = 0;
            while (x < S)
            {
                BlockType t = mask[y * S + x];
                if (t == BlockType::AIR) { x++; continue; }

                int w = 1;
                while (x + w < S && mask[y * S + (x + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (y + h < H && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(y + h) * S + (x + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dy = 0; dy < h; ++dy)
                    for (int dx = 0; dx < w; ++dx)
                        mask[(y + dy) * S + (x + dx)] = BlockType::AIR;

                float zz = m_Position.z + z + 0.5f;
                float x0 = m_Position.x + x - 0.5f;
                float x1 = m_Position.x + x + w - 0.5f;
                float y0 = m_Position.y + y - 0.5f;
                float y1 = m_Position.y + y + h - 0.5f;

                glm::vec3 n(0.0f, 0.0f, 1.0f);

                glm::vec3 v0(x0, y0, zz);
                glm::vec3 v1(x1, y0, zz);
                glm::vec3 v2(x1, y1, zz);
                glm::vec3 v3(x0, y1, zz);

                pushQuad(v0, v1, v2, v3, n);

                x += w;
            }
            y++;
        }
    }

    for (int z = 0; z < S; ++z)
    {
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < S; ++x)
            {
                BlockType t = GetBlockFast(x, y, z).GetType();
                bool solid = (t != BlockType::AIR);
                bool neighborSolid = IsSolid(x, y, z - 1);

                mask[y * S + x] = (solid && !neighborSolid) ? t : BlockType::AIR;
            }
        }

        int y = 0;
        while (y < H)
        {
            int x = 0;
            while (x < S)
            {
                BlockType t = mask[y * S + x];
                if (t == BlockType::AIR) { x++; continue; }

                int w = 1;
                while (x + w < S && mask[y * S + (x + w)] == t) w++;

                int h = 1;
                bool stop = false;
                while (y + h < H && !stop)
                {
                    for (int k = 0; k < w; ++k)
                    {
                        if (mask[(y + h) * S + (x + k)] != t)
                        {
                            stop = true; break;
                        }
                    }
                    if (!stop) h++;
                }

                for (int dy = 0; dy < h; ++dy)
                    for (int dx = 0; dx < w; ++dx)
                        mask[(y + dy) * S + (x + dx)] = BlockType::AIR;

                float zz = m_Position.z + z - 0.5f;
                float x0 = m_Position.x + x + w - 0.5f;
                float x1 = m_Position.x + x - 0.5f;
                float y0 = m_Position.y + y - 0.5f;
                float y1 = m_Position.y + y + h - 0.5f;

                glm::vec3 n(0.0f, 0.0f, -1.0f);

                glm::vec3 v0(x0, y0, zz);
                glm::vec3 v1(x1, y0, zz);
                glm::vec3 v2(x1, y1, zz);
                glm::vec3 v3(x0, y1, zz);

                pushQuad(v0, v1, v2, v3, n);

                x += w;
            }
            y++;
        }
    }
}

void Chunk::ClearMesh()
{
	QuasarEngine::Entity entity{ entt_entity, registry };
	entity.GetComponent<QuasarEngine::MeshComponent>().ClearMesh();
	m_HeightTimer = 0.0f;
}

bool Chunk::IsMeshGenerated() const
{
	QuasarEngine::Entity entity{ entt_entity, registry };
	if (entity.GetComponent<QuasarEngine::MeshComponent>().HasMesh())
		return entity.GetComponent<QuasarEngine::MeshComponent>().GetMesh().IsMeshGenerated();
	else
		return false;
}

inline int Chunk::ToIndex(int x, int y, int z) const
{
	return y * CHUNK_AREA + z * CHUNK_SIZE + x;
}

inline int Chunk::ToIndex(const glm::ivec3& p) const
{
	return ToIndex(p.x, p.y, p.z);
}

inline const Block& Chunk::GetBlockFast(int x, int y, int z) const
{
	return m_Blocks[ToIndex(x, y, z)];
}

inline bool Chunk::IsSolid(int x, int y, int z) const
{
    if (x >= 0 && x < CHUNK_SIZE &&
        y >= 0 && y < CHUNK_HEIGHT &&
        z >= 0 && z < CHUNK_SIZE)
    {
        return GetBlockFast(x, y, z).GetType() != BlockType::AIR;
    }

    glm::ivec3 worldPos = m_Position + glm::ivec3(x, y, z);

    ChunkManager* cm = ChunkManager::GetInstance();
    if (!cm)
        return false;

    BlockType t = cm->GetBlockType(worldPos);

    return t != BlockType::AIR && t != BlockType::BLOCK_ERROR;
}