#pragma once

#include "../World/Blocks/BlockInfos.h"
#include "../Utils/Math.h"

#include "GameCamera.h"

#include <glm/glm.hpp>
#include <memory>

#include <QuasarEngine/Tools/Math.h>

class Player
{
private:
	struct RaycastHit
	{
		bool hit = false;
		glm::ivec3 blockPos;
		glm::ivec3 normal;
		BlockType type = BlockType::AIR;
	};

	//QuasarEngine::UUID m_uuid;

	int scroll = 1;

	float lastMouseX = 0;
	float lastMouseY = 0;

	std::unique_ptr<GameCamera> m_Camera;

public:
	Player();
	~Player();

	void Update(float dt);

	RaycastHit VoxelRaycast(const glm::vec3& origin, const glm::vec3& dir, float maxDistance);

	const glm::vec3& GetPosition() const;

	GameCamera& GetCamera();

	//QuasarEngine::UUID GetUUID() const;
};

