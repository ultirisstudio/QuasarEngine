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
#include <QuasarEngine/Tools/Math.h>

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
        RaycastHit raycast = VoxelRaycast(position + 0.5f, m_Camera->GetFront(), INFINITY);

        if (raycast.hit && raycast.type != BlockType::BLOCK_ERROR && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.blockPos)) != nullptr && ChunkManager::GetInstance()->GetChunk(Math::ToChunkPosition(raycast.blockPos))->IsMeshGenerated())
        {
            ChunkManager::GetInstance()->SetBlock(raycast.blockPos, BlockType::AIR);
        }
    }

    if (QuasarEngine::Input::IsMouseButtonPressed(QuasarEngine::Mouse::ButtonRight))
    {
        RaycastHit raycast = VoxelRaycast(position + 0.5f, m_Camera->GetFront(), INFINITY);

        if (raycast.hit && raycast.type != BlockType::BLOCK_ERROR)
        {
            glm::ivec3 placePos = raycast.blockPos + raycast.normal;

            glm::ivec3 chunkPos = Math::ToChunkPosition(placePos);
            Chunk* chunk = ChunkManager::GetInstance()->GetChunk(chunkPos);

            if (chunk && chunk->IsMeshGenerated())
            {
                ChunkManager::GetInstance()->SetBlock(placePos, BlockType(scroll));
            }
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

Player::RaycastHit Player::VoxelRaycast(const glm::vec3& origin, const glm::vec3& dir, float maxDistance)
{
    RaycastHit result;

    glm::vec3 rayDir = glm::normalize(dir);

    glm::ivec3 voxel = glm::floor(origin);
    glm::vec3 tMax;
    glm::vec3 tDelta;
    glm::ivec3 step;

    for (int i = 0; i < 3; ++i)
    {
        if (rayDir[i] > 0)
        {
            step[i] = 1;
            float nextVoxel = voxel[i] + 1.0f;
            tMax[i] = (nextVoxel - origin[i]) / rayDir[i];
            tDelta[i] = 1.0f / rayDir[i];
        }
        else if (rayDir[i] < 0)
        {
            step[i] = -1;
            float nextVoxel = voxel[i];
            tMax[i] = (nextVoxel - origin[i]) / rayDir[i];
            tDelta[i] = -1.0f / rayDir[i];
        }
        else
        {
            step[i] = 0;
            tMax[i] = std::numeric_limits<float>::infinity();
            tDelta[i] = std::numeric_limits<float>::infinity();
        }
    }

    float dist = 0.0f;
    glm::ivec3 lastStep(0);

    while (dist <= maxDistance)
    {
        BlockType type = ChunkManager::GetInstance()->GetBlockType(voxel);
        if (type != BlockType::AIR && type != BlockType::BLOCK_ERROR)
        {
            result.hit = true;
            result.blockPos = voxel;
            result.normal = -lastStep;
            result.type = type;
            return result;
        }

        int axis = 0;
        if (tMax.y < tMax.x) axis = 1;
        if (tMax.z < tMax[axis]) axis = 2;

        voxel[axis] += step[axis];
        dist = tMax[axis];
        tMax[axis] += tDelta[axis];
        lastStep = glm::ivec3(0);
        lastStep[axis] = step[axis];
    }

    return result;
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
