#pragma once

#include "QuasarEngine/Core/Core.h"

//#include <memory>
//#include <unordered_map>
//#include <glm/glm.hpp>

namespace QuasarEngine
{
	extern Application* CreateApplication(ApplicationCommandLineArgs args);
}

int main(int argc, char** argv)
{
	std::unique_ptr<QuasarEngine::Application> app(QuasarEngine::CreateApplication({ argc, argv }));
	app->Run();

	/*enum class OrganismType
	{
		PLANT,
		ANIMAL
	};

	class Organism
	{
	public:
		float energy;

		virtual OrganismType getType() const = 0;
	};

	class Animal : public Organism
	{
	public:
		float speed;

		OrganismType getType() const override { return OrganismType::ANIMAL; }
	};

	class Plant : public Organism
	{
	public:
		float growthRate;

		OrganismType getType() const override { return OrganismType::PLANT; }
	};

	std::unordered_map<glm::vec2, std::vector<std::unique_ptr<Organism>>> map;

	map.reserve(5 * 5);
	
	for (int x = 0; x < 5; ++x)
	{
		for (int y = 0; y < 5; ++y)
		{
			glm::vec2 position = glm::vec2(x, y);
			map[position].reserve(2);
		}
	}

	map[glm::vec2(2, 3)].push_back(std::make_unique<Animal>());

	if (map.find(glm::vec2(2, 3)) != map.end())
	{
		auto& organisms = map[glm::vec2(2, 3)];

		if (organisms.at(0) != nullptr && organisms.at(0)->getType() == OrganismType::ANIMAL)
		{
			
		}
	}*/

	return 0;
}