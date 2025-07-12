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
	struct RaycastResult
	{
		bool hit;
		BlockInfo block;
		glm::vec3 pos;
		QuasarEngine::Math::Direction normal;
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

	RaycastResult Raycast(glm::vec3 pos, glm::vec3 dir, float length = INFINITY);

	const glm::vec3& GetPosition() const;

	GameCamera& GetCamera();

	//QuasarEngine::UUID GetUUID() const;
};

