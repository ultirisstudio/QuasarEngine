#include "TerrainGenerator.h"
#include "../Biomes/BiomeInfos.h"
#include "../Chunks/ChunkManager.h"

#include <fstream>
#include <random>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/spline.hpp>
#include <string>

TerrainGenerator::TerrainGenerator() : seed(8173561), perlin()
{
	continetalnessCurve = loadCurve("Assets/Curves/continentalness.csv");
	erosionCurve = loadCurve("Assets/Curves/erosion.csv");
	peaksAndValleyCurve = loadCurve("Assets/Curves/peaks_and_valleys.csv");
}

TerrainGenerator::~TerrainGenerator()
{

}

std::vector<glm::vec2> TerrainGenerator::loadCurve(const std::string& path)
{
	std::ifstream file(path);
	std::string line;

	std::vector<glm::vec2> controlPoints;

	while (std::getline(file, line))
	{
		size_t separatorPosition = line.find(',');

		glm::vec2 point{ std::stof(line.substr(0, separatorPosition)), std::stof(line.substr(separatorPosition + 1)) };
		controlPoints.push_back(point);
	}

	return controlPoints;
}

int TerrainGenerator::GetHeight(glm::vec2 pos)
{
	float continentalnessNoise = perlin.octave2D(pos.x * 0.0009f, pos.y * 0.0009f, 10);
	float erosionNoise = perlin.octave2D(pos.x * 0.0038f, pos.y * 0.0038f, 5);
	float peaksAndValleysNoise = perlin.octave2D(pos.x * 0.001f, pos.y * 0.001f, 10);

	float continentalness = QuasarEngine::Math::MapRange(GetY(continentalnessNoise, pos, continetalnessCurve), 0.0f, 255.0f, 0.0f, 255.0f);
	float erosion = QuasarEngine::Math::MapRange(GetY(erosionNoise, pos, erosionCurve), 0.0f, 255.0f, 0.0f, 255.0f);
	float peaksAndValley = QuasarEngine::Math::MapRange(GetY(peaksAndValleysNoise, pos, peaksAndValleyCurve), 0.0f, 255.0f, 0.0f, 255.0f);

	std::vector<glm::vec2> positions;
	int valC = static_cast<int>(continentalness);
	int valE = static_cast<int>(erosion);
	int valPV = static_cast<int>(peaksAndValley);

	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			if (pos.x == 0 && pos.y == 0)
				continue;

			positions.push_back(glm::vec2(pos.x + i, pos.y + j));
		}
	}

	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		float c = QuasarEngine::Math::MapRange(GetY(continentalnessNoise, positions[i], continetalnessCurve), 0.0f, 255.0f, 0.0f, 255.0f);
		float e = QuasarEngine::Math::MapRange(GetY(erosionNoise, positions[i], erosionCurve), 0.0f, 255.0f, 0.0f, 255.0f);
		float pv = QuasarEngine::Math::MapRange(GetY(peaksAndValleysNoise, positions[i], peaksAndValleyCurve), 0.0f, 255.0f, 0.0f, 150.0f);

		valC += static_cast<int>(c);
		valE += static_cast<int>(e);
		valPV += static_cast<int>(pv);
	}

	valC /= 9.0f;
	valE /= 9.0f;
	valPV /= 9.0f;

	int surfaceHeight = peaksAndValley;

	/*if (continentalnessNoise > 0.17f)
	{
		int delta = 0;

		if (valC > valPV)
			delta = valPV - valC;
		else
			delta = valC - valPV;

		surfaceHeight = valC + delta;

		if (erosionNoise > 0.0f)
		{
			surfaceHeight = valPV;
		}
		else
		{
			surfaceHeight = valE;
		}
	}
	else
	{
		surfaceHeight = valC;
	}*/

	return surfaceHeight;
}

float TerrainGenerator::GetY(float noise_value, const glm::vec2& pos, const std::vector<glm::vec2>& list)
{
	noise_value = QuasarEngine::Math::MapRange(noise_value, -1.0f, 1.0f, list.front().x, list.back().x);

	std::vector<glm::vec2> points(4);

	glm::vec2 leftPoint = GetLeftPoint(list, noise_value);
	glm::vec2 rightPoint = GetRightPoint(list, noise_value);

	points[0] = GetLeftPoint(list, leftPoint.x);
	points[1] = leftPoint;
	points[2] = rightPoint;
	points[3] = GetRightPoint(list, rightPoint.x);

	float value = QuasarEngine::Math::MapRange(noise_value - points[1].x, 0.0f, points[2].x - points[1].x, 0.0f, 1.0f);
	//float y = glm::catmullRom(points[0], points[1], points[2], points[3], value).y;
	//float y = interpolator.LinearInterpolation(value, points[1], points[2]).y;
	float y = QuasarEngine::Interpolator::CatmullRomInterpolation(value, points[0], points[1], points[2], points[3]).y;

	return y;
}

const glm::vec2& TerrainGenerator::GetLeftPoint(const std::vector<glm::vec2>& points, float x) const
{
	unsigned int index = 0;

	for (int i = 0; i < points.size() - 1; i++)
	{
		if (points[i].x < x)
			index = i;
		else
			break;
	}

	return points[index];
}

const glm::vec2& TerrainGenerator::GetRightPoint(const std::vector<glm::vec2>& points, float x) const
{
	unsigned int index = static_cast<unsigned int>(points.size() - 1);

	for (int i = static_cast<int>(points.size()) - 1; i >= 0; i--)
	{
		if (points[i].x > x)
			index = i;
		else
			break;
	}

	return points[index];
}

float TerrainGenerator::GetNoiseHeight(glm::vec2 pos)
{
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(0, 10000);

    int seed = distribution(generator);

	BiomeInfos infos = ChunkManager::GetInstance()->GetBiomeInfos(GetBiome(pos));

	float height = ((glm::simplex(pos / float(HEIGHT_SCALE)) + 1) / 2) * infos.GetHeightWeight() * MAX_HEIGHT +
		((glm::simplex(pos / float(infos.GetDetailScale())) + 1) / 2) * infos.GetDetailWeight() * infos.GetDetailMaxHeight();

	height *= (glm::clamp(
		(glm::simplex(pos / static_cast<float>(LAND_SCALE)) + infos.GetMountainBias() * 2.0f) * LAND_TRANSITION_SHARPNESS,
		-1.0f + LAND_MIN_MULT * 2.0f,
		1.0f
	) + 1.0f) / 2.0f;

	/*
	float height = ((glm::simplex(pos / float(HEIGHT_SCALE)) + 1) / 2) * HEIGHT_WEIGHT * MAX_HEIGHT +
		((glm::simplex(pos / float(DETAIL_SCALE)) + 1) / 2) * DETAIL_WEIGHT * DETAIL_MAX_HEIGHT;

	height *= (glm::clamp(
		(glm::simplex(pos / static_cast<float>(LAND_SCALE)) + LAND_MONTAINS_BIAS * 2.0f) * LAND_TRANSITION_SHARPNESS,
		-1.0f + LAND_MIN_MULT * 2.0f,
		1.0f
	) + 1.0f) / 2.0f;
	*/

	height += MIN_HEIGHT;

	return height;
}

float TerrainGenerator::GetPerlin(glm::vec2 pos)
{
	return glm::perlin(glm::vec2(pos.x, pos.y));
}

void TerrainGenerator::GenerateCave()
{
	
}

void TerrainGenerator::GenerateTree(Chunk& chunk, int x, int y, int z)
{
    
	//int height = std::rand() % 6 + 6;

	for (int j = 0; j < 6; j++)
	{
		Block temp = Block(BlockType::LOG);
		chunk.SetBlock(glm::u8vec3(x, y + j, z), temp);
	}

	std::vector<glm::vec3> leaves;

	leaves.push_back(glm::vec3(x + 1,	y + 3, z));
	leaves.push_back(glm::vec3(x + 2,	y + 3, z));
	leaves.push_back(glm::vec3(x + 3,	y + 3, z));
	
	leaves.push_back(glm::vec3(x - 1,	y + 3, z));
	leaves.push_back(glm::vec3(x - 2,	y + 3, z));
	leaves.push_back(glm::vec3(x - 3,	y + 3, z));
	
	leaves.push_back(glm::vec3(x,		y + 3, z + 1));
	leaves.push_back(glm::vec3(x,		y + 3, z + 2));
	leaves.push_back(glm::vec3(x,		y + 3, z + 3));
	
	leaves.push_back(glm::vec3(x,		y + 3, z - 1));
	leaves.push_back(glm::vec3(x,		y + 3, z - 2));
	leaves.push_back(glm::vec3(x,		y + 3, z - 3));
	
	leaves.push_back(glm::vec3(x + 1,	y + 3, z + 1));
	leaves.push_back(glm::vec3(x + 2,	y + 3, z + 1));
	leaves.push_back(glm::vec3(x + 1,	y + 3, z + 2));
	
	leaves.push_back(glm::vec3(x - 1,	y + 3, z - 1));
	leaves.push_back(glm::vec3(x - 2,	y + 3, z - 1));
	leaves.push_back(glm::vec3(x - 1,	y + 3, z - 2));
	
	leaves.push_back(glm::vec3(x + 1,	y + 3, z - 1));
	leaves.push_back(glm::vec3(x + 2,	y + 3, z - 1));
	leaves.push_back(glm::vec3(x + 1,	y + 3, z - 2));
	
	leaves.push_back(glm::vec3(x - 1,	y + 3, z + 1));
	leaves.push_back(glm::vec3(x - 2,	y + 3, z + 1));
	leaves.push_back(glm::vec3(x - 1,	y + 3, z + 2));
	
	leaves.push_back(glm::vec3(x + 1,	y + 4, z));
	leaves.push_back(glm::vec3(x - 1,	y + 4, z));
	leaves.push_back(glm::vec3(x,		y + 4, z + 1));
	leaves.push_back(glm::vec3(x,		y + 4, z - 1));
	
	leaves.push_back(glm::vec3(x + 1,	y + 5, z));
	leaves.push_back(glm::vec3(x + 2,	y + 5, z));
	leaves.push_back(glm::vec3(x - 1,	y + 5, z));
	leaves.push_back(glm::vec3(x - 2,	y + 5, z));
	leaves.push_back(glm::vec3(x,		y + 5, z + 1));
	leaves.push_back(glm::vec3(x,		y + 5, z + 2));
	leaves.push_back(glm::vec3(x,		y + 5, z - 1));
	leaves.push_back(glm::vec3(x,		y + 5, z - 2));
	
	leaves.push_back(glm::vec3(x + 1,	y + 5, z + 1));
	leaves.push_back(glm::vec3(x - 1,	y + 5, z - 1));
	leaves.push_back(glm::vec3(x + 1,	y + 5, z - 1));
	leaves.push_back(glm::vec3(x - 1,	y + 5, z + 1));
	
	leaves.push_back(glm::vec3(x,		y + 6, z));
	leaves.push_back(glm::vec3(x + 1,	y + 6, z));
	leaves.push_back(glm::vec3(x - 1,	y + 6, z));
	leaves.push_back(glm::vec3(x,		y + 6, z + 1));
	leaves.push_back(glm::vec3(x,		y + 6, z - 1));

	leaves.push_back(glm::vec3(x, y + 7, z));

	for (int i = 0; i < leaves.size(); i++)
	{
		Block temp = Block(BlockType::LEAVES);
		chunk.SetBlock(leaves[i], temp);
	}
}

BiomeType TerrainGenerator::GetBiome(glm::vec2 pos)
{
	return BiomeType::MONTAINS;
}
