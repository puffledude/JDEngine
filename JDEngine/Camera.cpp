#include "Camera.h"
#include <glm/gtc/quaternion.hpp>


namespace JD {
	Camera::Camera(glm::vec3 position, Controller* controller, float yaw, float pitch) :
		Position(position), controller(controller), Yaw(yaw), Pitch(pitch), Front(glm::vec3(0.0f, 0.0f, -1.0f)), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
	{
		updateCameraVectors();
	}

	glm::mat4 Camera::GetViewMatrix() {
		// Ensure direction vectors are up-to-date, then build view matrix
		return glm::lookAt(Position, Position + Front, Up);
	}

	void Camera::Update(float dt) {
		glm::vec2 leftStickDir = controller->getLeftStickDir();

		Position += Front * leftStickDir.y * speed * dt;
		Position += Right * leftStickDir.x * speed * dt;

		glm::vec2 rightStickDir = controller->getRightStickDir();
		Yaw += rightStickDir.x * sensitivity * dt;
		Pitch -= rightStickDir.y * sensitivity * dt;

		if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;
		if (Yaw > 360.0f) Yaw -= 360.0f;
		if (Yaw < 0.0f) Yaw += 360.0f;
		updateCameraVectors();
	}


	void Camera::updateCameraVectors() {
		// Convert degrees to radians for trig functions
		const float yawRad = glm::radians(Yaw);
		const float pitchRad = glm::radians(Pitch);

		// Calculate the new Front vector
		Front.x = cos(yawRad) * cos(pitchRad);
		Front.y = sin(pitchRad);
		Front.z = sin(yawRad) * cos(pitchRad);
		Front = glm::normalize(Front);

		// Recalculate Right and Up vectors
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
