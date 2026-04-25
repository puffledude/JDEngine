#include "Camera.h"
#include <glm/gtc/quaternion.hpp>


namespace JD {
	Camera::Camera(glm::vec3 position, float yaw, float pitch) :
		Position(position), Yaw(yaw), Pitch(pitch), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
	{
		updateCameraVectors();
	}

	glm::mat4 Camera::GetViewMatrix() {
		// Ensure direction vectors are up-to-date, then build view matrix
		updateCameraVectors();
		return glm::lookAt(Position, Position + Front, Up);
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
