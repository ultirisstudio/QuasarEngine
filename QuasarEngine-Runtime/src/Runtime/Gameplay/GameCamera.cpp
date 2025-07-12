#include "GameCamera.h"

#include <QuasarEngine/Core/Window.h>
#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Input.h>

GameCamera::GameCamera() :
	m_viewMatrix(glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))),
	m_projectionMatrix(1.0f),

	m_yaw(-90.0f),
	m_pitch(0.0f),
	m_position(glm::vec3(0.0f, 0.0f, 0.0f)),

	m_target(glm::vec3(0.0f, 0.0f, -1.0f)),
	m_worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),

	m_fov(45.0f),
	m_minFov(15.0f),
	m_maxFov(95.0f),

	m_lastMousePos(0),
	m_canMove(true),

	m_RotateSensitivity(0.1f),
	m_TranslateSensitivity(0.1f),
	m_WalkSensitivity(0.4f),
	m_ScrollSensitivity(3.0f),

	m_MouseX(0.0f),
	m_MouseY(0.0f)
{
	UpdateCameraVectors();
}

GameCamera::GameCamera(const glm::vec3& position) :
	m_viewMatrix(glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))),
	m_projectionMatrix(1.0f),

	m_yaw(-90.0f),
	m_pitch(0.0f),
	m_position(position),

	m_target(glm::vec3(0.0f, 0.0f, -1.0f)),
	m_worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),

	m_fov(45.0f),
	m_minFov(15.0f),
	m_maxFov(95.0f),

	m_lastMousePos(0),
	m_canMove(true),

	m_RotateSensitivity(0.1f),
	m_TranslateSensitivity(0.1f),
	m_WalkSensitivity(0.4f),
	m_ScrollSensitivity(3.0f),

	m_MouseX(0.0f),
	m_MouseY(0.0f)
{
	UpdateCameraVectors();
}

void GameCamera::updateViewMatrix()
{
	m_viewMatrix = glm::lookAt(m_position, m_position + m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

const glm::mat4 GameCamera::getViewMatrix() const
{
	return m_viewMatrix;
}

const glm::mat4& GameCamera::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

glm::vec3 GameCamera::GetFront()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	return glm::normalize(front);
}

glm::vec3 GameCamera::GetPosition()
{
	return m_position;
}

glm::mat4 GameCamera::GetTransform()
{
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_pitch), { 1, 0, 0 }) * glm::rotate(glm::mat4(1.0f), glm::radians(m_yaw), { 0, 1, 0 }) * glm::rotate(glm::mat4(1.0f), 0.0f, { 0, 0, 1 });
	return glm::translate(glm::mat4(1.f), m_position) * rotation;
}

float GameCamera::getFov() const
{
	return m_fov;
}

void GameCamera::UpdateCameraVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_target = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_target, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_target));
	m_forward = glm::normalize(glm::cross(m_right, m_up));
}

void GameCamera::Update()
{
	if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::W))
	{
		glm::mat4 translationMatrix(1.0f);
		translationMatrix = glm::translate(translationMatrix, m_target * (1 * m_WalkSensitivity));

		m_position = (translationMatrix * glm::vec4(m_position, 1.0f));

		updateViewMatrix();
		UpdateCameraVectors();
	}

	if (QuasarEngine::Input::IsKeyPressed(QuasarEngine::Key::S))
	{
		glm::mat4 translationMatrix(1.0f);
		translationMatrix = glm::translate(translationMatrix, m_target * (-1 * m_WalkSensitivity));

		m_position = (translationMatrix * glm::vec4(m_position, 1.0f));

		updateViewMatrix();
		UpdateCameraVectors();
	}
}

void GameCamera::OnResize(float width, float height)
{
	m_ViewportSize.x = width;
	m_ViewportSize.y = height;
	m_projectionMatrix = glm::perspective(glm::radians(getFov()), m_ViewportSize.x / m_ViewportSize.y, 0.1f, 1000.0f);
	//QuasarEngine::RenderCommand::SetViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
}

void GameCamera::OnEvent(QuasarEngine::Event& e)
{
	QuasarEngine::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<QuasarEngine::MouseMovedEvent>(std::bind(&GameCamera::OnMouseMoved, this, std::placeholders::_1));
	dispatcher.Dispatch<QuasarEngine::MouseScrolledEvent>(std::bind(&GameCamera::OnMouseScrolled, this, std::placeholders::_1));
	dispatcher.Dispatch<QuasarEngine::MouseButtonPressedEvent>(std::bind(&GameCamera::OnMousePressed, this, std::placeholders::_1));
	dispatcher.Dispatch<QuasarEngine::MouseButtonReleasedEvent>(std::bind(&GameCamera::OnMouseReleased, this, std::placeholders::_1));
}

bool GameCamera::OnMouseMoved(QuasarEngine::MouseMovedEvent& e)
{
	if (!m_canMove)
		return false;

	float offsetX = static_cast<float>(m_lastMousePos.x - e.GetX());
	float offsetY = static_cast<float>(m_lastMousePos.y - e.GetY());

	m_lastMousePos.x = static_cast<glm::uint>(e.GetX());
	m_lastMousePos.y = static_cast<glm::uint>(e.GetY());

	m_yaw -= (offsetX * m_RotateSensitivity);
	m_pitch += (offsetY * m_RotateSensitivity);

	m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

	if (m_yaw < 0.0f)
		m_yaw += 360.0f;
	else if (m_yaw >= 360.0f)
		m_yaw = std::fmod(m_yaw, 360.0f);

	updateViewMatrix();
	UpdateCameraVectors();

	return false;
}

bool GameCamera::OnMousePressed(QuasarEngine::MouseButtonPressedEvent& e)
{
	m_lastMousePos.x = QuasarEngine::Input::GetMouseX();
	m_lastMousePos.y = QuasarEngine::Input::GetMouseY();

	return false;
}

bool GameCamera::OnMouseReleased(QuasarEngine::MouseButtonReleasedEvent& e)
{
	return false;
}

bool GameCamera::OnMouseScrolled(QuasarEngine::MouseScrolledEvent& e)
{
	glm::mat4 translationMatrix(1.0f);
	translationMatrix = glm::translate(translationMatrix, m_target * (e.GetYOffset() * m_ScrollSensitivity));

	m_position = (translationMatrix * glm::vec4(m_position, 1.0f));

	updateViewMatrix();
	UpdateCameraVectors();

	return false;
}