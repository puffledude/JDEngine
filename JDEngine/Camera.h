#pragma once
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include "Controller.h"

namespace JD
{
	class Camera {

		public:
			Camera(glm::vec3 position, Controller* controller,float yaw, float pitch);
			glm::vec3 GetPosition() const { return Position; }
			float GetPitch() const { return Pitch; }
			float GetYaw() const { return Yaw; }
			void Update(float dt);
			glm::mat4 GetViewMatrix();
			
	protected:
			glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 Front;
			glm::vec3 Up;
			glm::vec3 Right;
			glm::vec3 WorldUp;
			float Yaw;
			float Pitch;
			Controller* controller;
			void updateCameraVectors();
			float speed = 5.0f;
			float sensitivity =6.0f;

	};
}
