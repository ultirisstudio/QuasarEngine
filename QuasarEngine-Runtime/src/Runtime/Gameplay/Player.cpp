#include "Player.h"

#include "../World/Chunks/ChunkManager.h"

#include <QuasarEngine/Core/Input.h>
#include <QuasarEngine/Core/MouseCodes.h>
#include <QuasarEngine/Core/KeyCodes.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>

float deltaTime = 0;
float lastFrame = 0;

Player::Player()
{
	//QuasarEngine::Entity player = QuasarEngine::Renderer::m_SceneData.m_Scene->CreateEntity("player");
	//player.AddComponent<QuasarEngine::CameraComponent>().GetCamera().Init(&player.GetComponent<QuasarEngine::TransformComponent>());
	//player.AddComponent<QuasarEngine::CameraComponent>().Primary = true;
	//player.AddComponent<QuasarEngine::CameraComponent>().setType(QuasarEngine::CameraType::PERSPECTIVE);
	//player.GetComponent<QuasarEngine::TransformComponent>().Position.y = 200;

	m_Camera = std::make_unique<GameCamera>(glm::vec3(0.0f, 300.0f, 0.0f));
	m_Camera->setFov(90.0f);

	//m_uuid = player.GetUUID();

	lastMouseX = QuasarEngine::Input::GetMouseX();
	lastMouseY = QuasarEngine::Input::GetMouseY();
}

Player::~Player()
{
}

void Player::Update(float dt)
{
	glm::vec3 position = m_Camera->GetPosition();

    if (QuasarEngine::Input::IsMouseButtonPressed(QuasarEngine::Mouse::ButtonLeft))
    {
        RaycastResult raycast = Raycast(position + 0.5f, m_Camera->GetFront(), INFINITY);

        if (raycast.hit && raycast.block.second.GetType() != BlockType::BLOCK_ERROR && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.pos)) != nullptr && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.pos))->IsMeshGenerated())
        {
            ChunkManager::GetInstance()->SetBlock(raycast.block.first, BlockType::AIR);
        }
    }

    if (QuasarEngine::Input::IsMouseButtonPressed(QuasarEngine::Mouse::ButtonRight))
    {
		RaycastResult raycast = Raycast(position + 0.5f, m_Camera->GetFront(), INFINITY);

        if (raycast.hit && raycast.block.second.GetType() != BlockType::BLOCK_ERROR && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.pos)) != nullptr && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.pos))->IsMeshGenerated())
        {
            ChunkManager::GetInstance()->SetBlock(raycast.block.first + glm::ivec3(QuasarEngine::Math::directionVectors[raycast.normal]), BlockType(scroll));
        }
    }

	//QuasarEngine::Entity light = QuasarEngine::Renderer::m_SceneData.m_Scene->FindEntityByName("PlayerLight");
	//light.GetComponent<QuasarEngine::TransformComponent>().Position = m_Camera->GetPosition();

    //m_Camera->ProcessMouseMovement(-InputManager::GetInstance()->GetDeltaMouse().x, InputManager::GetInstance()->GetDeltaMouse().y);

	/*float dx = QuasarEngine::Input::GetMouseX() - lastMouseX;
	float dy = QuasarEngine::Input::GetMouseY() - lastMouseY;

	if (dx != 0)
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Rotation.y -= dx * 0.1;

	if (dy != 0)
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Rotation.x -= dy * 0.1;

	lastMouseX = QuasarEngine::Input::GetMouseX();
	lastMouseY = QuasarEngine::Input::GetMouseY();

	if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::W))
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position.x += 0.1;
    if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::S))
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position.x -= 0.1;
    if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::A))
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position.z += 0.1;
    if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::D))
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position.z -= 0.1;
	if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::Space))
		QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position.y += 0.1;*/
}

Player::RaycastResult Player::Raycast(glm::vec3 pos, glm::vec3 dir, float length)
{
	dir = glm::normalize(dir);

	{
		glm::ivec3 coord = glm::floor(pos);
		Block block = ChunkManager::GetInstance()->GetBlockType(coord);
		if (block.GetType() != BlockType::AIR)
		{
			RaycastResult result;
			result.hit = true;
			result.block = BlockInfo(coord, block);
			result.pos = pos;
			result.normal = QuasarEngine::Math::VectorToDir(-dir);
			return result;
		}
	}

	while (true)
	{
		float dists[QuasarEngine::Math::AXIS_COUNT];
		float min = INFINITY;
		int axis;

		for (int i = 0; i < QuasarEngine::Math::AXIS_COUNT; i++)
		{
			dists[i] = Math::DistToBlock(pos, QuasarEngine::Math::Axis(i), dir);
			dists[i] *= glm::abs(1.f / dir[i]);
			if (dists[i] < min)
			{
				min = dists[i];
				axis = i;
			}
		}

		length -= min;

		if (length <= 0)
		{
			RaycastResult result;
			result.hit = false;
			return result;
		}

		pos += dir * min;

		glm::ivec3 blockCoord = glm::floor(pos);

		if (dir[axis] < 0.0f)
			blockCoord[axis]--;

		Block block = ChunkManager::GetInstance()->GetBlockType(blockCoord);

		if (block.GetType() != BlockType::AIR)
		{
			RaycastResult result;
			result.hit = true;
			result.block = BlockInfo(blockCoord, block);
			result.pos = pos;
			result.normal = QuasarEngine::Math::AxisToDir(QuasarEngine::Math::Axis(axis), !(dir[axis] < 0.0f));
			return result;
		}
	}
}

const glm::vec3& Player::GetPosition() const
{
	//return QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::TransformComponent>().Position;
	return m_Camera->GetPosition();
}

GameCamera& Player::GetCamera()
{
	//return QuasarEngine::Renderer::m_SceneData.m_Scene->GetEntityByUUID(m_uuid).GetComponent<QuasarEngine::CameraComponent>().GetCamera();
	return *m_Camera;
}

/*QuasarEngine::UUID Player::GetUUID() const
{
	return m_uuid;
}*/
