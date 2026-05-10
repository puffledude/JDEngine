#include "Camera.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace JD {
	Camera::Camera(glm::vec3 position, Controller* controller, float yaw, float pitch) :
		Position(position), controller(controller), Yaw(yaw), Pitch(pitch), Front(glm::vec3(0.0f, 0.0f, -1.0f)), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
	{
	
		updateCameraVectors();

		positionRail = {
		glm::vec3(-0.210491, -2.08042, -6.73261),
		glm::vec3(6.66763, -2.33138, -3.38146),
		glm::vec3(10.5382, -2.33138, 6.24414),
		glm::vec3(3.69712, -2.62783, 13.3314),
		glm::vec3(-5.61354, -2.62783, 12.4939),
		glm::vec3(-9.3939, -2.62783, 3.77541),
		glm::vec3(-4.14616, -2.62783, -4.40198),
		glm::vec3(-3.41716, -0.604192, -0.875155),
		glm::vec3(3.65245, 2.74069, -0.521407),
		glm::vec3(2.72436, -3.03723, 9.79991)
		};
		lookAtRail = {
		std::pair<float, float>(-11.6811, 88.6517),
		std::pair<float, float>(-13.7665, 125.696),
		std::pair<float, float>(-11.2355, 190.443),
		std::pair<float, float>(-16.5749, 250.067),
		std::pair<float, float>(-17.9567, 311.328),
		std::pair<float, float>(-14.8533, 7.3047),
		std::pair<float, float>(-18.9433, 67.4402),
		std::pair<float, float>(-32.0161, 57.2368),
		std::pair<float, float>(-41.3207, 125.504),
		std::pair<float, float>(-14.2033, 238.031)
		};

	}


	/*Camera Position : (-0.210491, -2.08042, -6.73261)
		Camera Rotation : (Pitch : -11.6811, Yaw : 88.6517)
		Camera Position : (6.66763, -2.33138, -3.38146)
		Camera Rotation : (Pitch : -13.7665, Yaw : 125.696)
		Camera Position : (10.5382, -2.33138, 6.24414)
		Camera Rotation : (Pitch : -11.2355, Yaw : 190.443)
		Camera Position : (3.69712, -2.62783, 13.3314)
		Camera Rotation : (Pitch : -16.5749, Yaw : 250.067)
		Camera Position : (-5.61354, -2.62783, 12.4939)
		Camera Rotation : (Pitch : -17.9567, Yaw : 311.328)
		Camera Position : (-9.3939, -2.62783, 3.77541)
		Camera Rotation : (Pitch : -14.8533, Yaw : 7.3047)
		Camera Position : (-4.14616, -2.62783, -4.40198)
		Camera Rotation : (Pitch : -18.9433, Yaw : 67.4402)
		Camera Position : (-3.41716, -0.604192, -0.875155)
		Camera Rotation : (Pitch : -32.0161, Yaw : 57.2368)
		Camera Position : (3.65245, 2.74069, -0.521407)
		Camera Rotation : (Pitch : -41.3207, Yaw : 125.504)
		Camera Position : (2.72436, -3.03723, 9.79991)
		Camera Rotation : (Pitch : -14.2033, Yaw : 238.031)*/




	glm::mat4 Camera::GetViewMatrix() {
		// Ensure direction vectors are up-to-date, then build view matrix
		return glm::lookAt(Position, Position + Front, Up);
	}

	void Camera::Update(float dt) {
		glm::vec2 leftStickDir = controller->getLeftStickDir();
		if (glm::length(leftStickDir) > 0.1f)  // Deadzone check
		{
			Position += Front * leftStickDir.y * speed * dt;
			Position += Right * leftStickDir.x * speed * dt;
			onRails = false;  // Exit rails if there's input
		}
		if (onRails) {
			// Move along the position rail
			if (currentRailIndex >= positionRail.size()) {
				currentRailIndex = 0;
			}
			glm::vec3 targetPos = positionRail[currentRailIndex];
			Position = glm::mix(Position, targetPos, dt);  // Smoothly interpolate to target position
			if (glm::length(Position - targetPos) < 0.1) {
				currentRailIndex++;
			}
			// Move along the lookAt rail
			if (currentRailIndex >= lookAtRail.size()) {
				currentRailIndex = 0;
			}
			std::pair<float, float> targetRot = lookAtRail[currentRailIndex];
			float targetPitch = targetRot.first;
			float rotDirx = targetPitch - this->Pitch;
			if (rotDirx > 180) rotDirx -= 360;
			if (rotDirx < -180) rotDirx += 360;
			Pitch = glm::mix(Pitch, rotDirx, dt);
			Yaw = glm::mix(Yaw, targetRot.second, dt);
			//glm::vec2 rotDir = targetRot - glm::vec2(this->Yaw, this->Pitch);
			
		}

		glm::vec2 rightStickDir = controller->getRightStickDir();
		Yaw += rightStickDir.x * sensitivity * dt;
		Pitch -= rightStickDir.y * sensitivity * dt;

		Pitch = std::min(Pitch, 90.0f);
		Pitch = std::max(Pitch, -90.0f);
	/*	if (rotDir.x > 180) rotDir.x -= 360;
		if (rotDir.x < -180) rotDir.x += 360;*/
		/*if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;*/
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
