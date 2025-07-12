#pragma once

#include <QuasarEngine/Scene/BaseCamera.h>

#include <QuasarEngine/Scene/Camera.h>
#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/MouseEvent.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

class GameCamera : public QuasarEngine::BaseCamera
{
private:
	glm::mat4 m_viewMatrix;
	glm::mat4 m_projectionMatrix;

	float m_yaw;
	float m_pitch;

	float m_fov;
	float m_minFov;
	float m_maxFov;

	glm::vec3 m_position;
	glm::vec3 m_target;
	glm::vec3 m_up;
	glm::vec3 m_right;
	glm::vec3 m_forward;
	glm::vec3 m_worldUp;
private:
	glm::uvec2 m_lastMousePos;

	bool m_canMove;

	float m_MouseX;
	float m_MouseY;

	float m_RotateSensitivity;
	float m_TranslateSensitivity;
	float m_WalkSensitivity;
	float m_ScrollSensitivity;

	glm::vec2 m_ViewportSize = { 1.0f, 1.0f };

	void updateViewMatrix();
public:
	GameCamera();
	GameCamera(const glm::vec3& position);

	const glm::mat4 getViewMatrix() const override;
	const glm::mat4& getProjectionMatrix() const override;

	glm::vec3 GetFront() override;

	glm::vec3 GetPosition();

	glm::mat4 GetTransform();

	float getFov() const;
	void setFov(float fov) { m_fov = fov; }

	void UpdateCameraVectors();
	void Update();
	void OnResize(float width, float height);
public:

	void freeze() { m_canMove = false; }
	void free() { m_canMove = true; }

	void OnEvent(QuasarEngine::Event& e);

	bool OnMouseMoved(QuasarEngine::MouseMovedEvent& e);
	bool OnMousePressed(QuasarEngine::MouseButtonPressedEvent& e);
	bool OnMouseReleased(QuasarEngine::MouseButtonReleasedEvent& e);
	bool OnMouseScrolled(QuasarEngine::MouseScrolledEvent& e);
};