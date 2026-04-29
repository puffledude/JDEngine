#include "GameWorld.h"
#include "Camera.h"
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
namespace JD
{
	Gameworld::Gameworld(JPH::PhysicsSystem* physicsSystem, Controller* controller) : physicsSystem(physicsSystem), controller(controller) {

		camera = new Camera(glm::vec3(0.0f, 0.0f, 0.0f), controller, 0.0f, 0.0f);
		registry = new entt::registry();
	}

	void Gameworld::Update(float dt) {
		camera->Update(dt);
	}

    glm::mat4 Gameworld::getCameraView() {
		// Return the camera view matrix by value. Returning a reference to a local variable would be undefined behavior.
		return camera->GetViewMatrix();
	}

	glm::vec3 Gameworld::getCameraPosition() {
		return camera->GetPosition();
	}

	glm::vec3 Gameworld::getSunPosition() {
		lightTransmition* sun = getSun();
		if (sun) {
			return sun->position;
		}
		return glm::vec3(0.0f); // Default position if no sun found
	}

	glm::mat4 Gameworld::getSunView() {
		//Need to calculate the front and up for the viewdir.
		lightTransmition* sun = getSun();
		sun->direction = glm::normalize(sun->direction);
		
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		// If the light's direction is nearly parallel to world up, switch the up vector
		if (std::abs(glm::dot(sun->direction, worldUp)) > 0.999f) {
			worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		glm::vec3 Right = glm::normalize(glm::cross(sun->direction, worldUp)); //Right = front x worldup
		glm::vec3 Up = glm::normalize(glm::cross(Right, sun->direction)); //Up = Right x front
		return glm::lookAt(sun->position, sun->position + sun->direction, Up);
	}

	Gameworld::~Gameworld() {		
		delete registry;
	}

	std::vector<RenderTransmition>* Gameworld::getRenderTransmitions()
	{
		const JPH::BodyLockInterface& lock_interface = physicsSystem->GetBodyLockInterface(); // Or GetBodyLockInterfaceNoLock

		std::vector<RenderTransmition>* renderTransmition = new std::vector<RenderTransmition>();
		auto view = registry->view<RenderableComponent, JoltComponent>();

		// Use .each() to smoothly unpack components by reference
		for (auto [entity, renderable, jolt] : view.each()) {

			RenderTransmition transmition;
			transmition.mesh = renderable.mesh;
			JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
			if (lock.Succeeded()) // body_id may no longer be valid
			{
                const JPH::Body& body = lock.GetBody();
				JPH::RVec3 position = body.GetPosition();
				JPH::Quat rotation = body.GetRotation();
				auto scaleView = registry->view<scaleComponent>();
				glm::vec3 scale(1.0f);
				if (scaleView.contains(entity)) {
					scaleComponent scaleComp = scaleView.get<scaleComponent>(entity);
					scale = scaleComp.scale;
				}
				glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(position.GetX(), position.GetY(), position.GetZ())) * glm::mat4_cast(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ())) * glm::scale(glm::mat4(1.0f), scale);
				transmition.modelMatrix = modelMatrix;
			}
			renderTransmition->push_back(transmition);
		}

		return renderTransmition;
	}
}
