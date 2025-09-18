#pragma once

#include <QuasarEngine/Scene/BaseCamera.h>

#include <QuasarEngine/Scene/Camera.h>
#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/MouseEvent.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine
{
	class EditorCamera : public BaseCamera
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
		bool m_usingGuizmo;
		bool m_moving;

		bool m_walk;
		bool m_rotate;
		bool m_translate;

		float m_MouseX;
		float m_MouseY;

		float m_RotateSensitivity;
		float m_TranslateSensitivity;
		float m_WalkSensitivity;
		float m_ScrollSensitivity;

		glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };

		void updateViewMatrix();
	public:
		EditorCamera(const glm::vec3& position);

		const glm::mat4& getViewMatrix() const override;
		const glm::mat4& getProjectionMatrix() const override;

		glm::vec3 GetFront() override;
		glm::mat4 GetTransform() override;
		glm::vec3 GetPosition() override;

		float getFov() const;
		void setFov(float fov) { m_fov = fov; }

		void UpdateCameraVectors();
		void Update();
		void OnResize(float width, float height);
	public:

		void freeze() { m_canMove = false; }
		void free() { m_canMove = true; }

		void useGuizmo() { m_usingGuizmo = true; }
		void unuseGuizmo() { m_usingGuizmo = false; }

		void OnEvent(Event& e);

		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnMousePressed(MouseButtonPressedEvent& e);
		bool OnMouseReleased(MouseButtonReleasedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

		bool m_CameraFocus;
	};
}