#pragma once

#include "../Chunks/Chunk.h"
#include "../Biomes/BiomeType.h"

#include <QuasarEngine/Tools/Math.h>
#include <QuasarEngine/Tools/PerlinNoise.h>

#include <glm/glm.hpp>

class TerrainGenerator
{
public:
	TerrainGenerator();
	~TerrainGenerator();
	
	int GetHeight(glm::vec2 pos);

	void GenerateCave();
	void GenerateTree(Chunk& chunk, int x, int y, int z);

	BiomeType GetBiome(glm::vec2 pos);
private:
	float GetNoiseHeight(glm::vec2 pos);
	float GetPerlin(glm::vec2 pos);

	float continentalnessNoise = 0;
	float erosionNoise = 0;
	float peaksAndValleyNoise = 0;

	std::vector<glm::vec2> loadCurve(const std::string& path);

	float GetY(float noise_value, const glm::vec2& pos, const std::vector<glm::vec2>& list);

	const glm::vec2& GetLeftPoint(const std::vector<glm::vec2>& points, float x) const;
	const glm::vec2& GetRightPoint(const std::vector<glm::vec2>& points, float x) const;

	std::vector<glm::vec2> continetalnessCurve;
	std::vector<glm::vec2> erosionCurve;
	std::vector<glm::vec2> peaksAndValleyCurve;

	const siv::PerlinNoise::seed_type seed;
	const siv::PerlinNoise perlin; //{seed}
};