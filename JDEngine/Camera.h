#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
namespace JD
{
	class Camera {

		public:
			Camera(glm::vec3 position, float yaw, float pitch);
			glm::vec3 GetPosition() const { return Position; }
			float GetPitch() const { return Pitch; }
			float GetYaw() const { return Yaw; }

			glm::mat4 GetViewMatrix();
			
	protected:
			glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 Front;
			glm::vec3 Up;
			glm::vec3 Right;
			glm::vec3 WorldUp;
			float Yaw;
			float Pitch;
			void updateCameraVectors();
	};
}
