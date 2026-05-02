#include "GameWorld.h"
#include "Camera.h"
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <execution>
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
		if (std::abs(glm::dot(glm::vec3(sun->direction), worldUp)) > 0.999f) {
			worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		glm::vec3 Right = glm::normalize(glm::cross(glm::vec3(sun->direction), worldUp)); //Right = front x worldup
		glm::vec3 Up = glm::normalize(glm::cross(Right, glm::vec3(sun->direction))); //Up = Right x front
		return glm::lookAt(glm::vec3(sun->position), glm::vec3(sun->position) + glm::vec3(sun->direction), Up);
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
		std::for_each(std::execution::par, view.each().begin(), view.each().end(), [&](auto&& tuple) {
			auto [entity, renderable, jolt] = tuple;
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
				renderTransmition->push_back(transmition);
			}});
			return renderTransmition;
	}
	//for (auto [entity, renderable, jolt] : view.each()) {

	//	RenderTransmition transmition;
	//	transmition.mesh = renderable.mesh;
	//	JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
	//	if (lock.Succeeded()) // body_id may no longer be valid
	//	{
//              const JPH::Body& body = lock.GetBody();
	  //		JPH::RVec3 position = body.GetPosition();
	  //		JPH::Quat rotation = body.GetRotation();
	  //		auto scaleView = registry->view<scaleComponent>();
	  //		glm::vec3 scale(1.0f);
	  //		if (scaleView.contains(entity)) {
	  //			scaleComponent scaleComp = scaleView.get<scaleComponent>(entity);
	  //			scale = scaleComp.scale;
	  //		}
	  //		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(position.GetX(), position.GetY(), position.GetZ())) * glm::mat4_cast(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ())) * glm::scale(glm::mat4(1.0f), scale);
	  //		transmition.modelMatrix = modelMatrix;
	  //	}
	  //	renderTransmition->push_back(transmition);
	  //}



	std::vector<lightTransmition>* Gameworld::getLighTransmitions() {
		std::vector<lightTransmition>* lightTransmitions = new std::vector<lightTransmition>();
		auto view = registry->view<lightComponent, JoltComponent>();
		const JPH::BodyLockInterface& lock_interface = physicsSystem->GetBodyLockInterface(); // Or GetBodyLockInterfaceNoLock
		auto colourView = registry->view<colourComponent>();
		auto directionView = registry->view<directionComponent>();


		std::for_each(std::execution::par, view.each().begin(), view.each().end(), [&](auto&& tuple) {
			auto [entity, light, jolt] = tuple;
			lightTransmition transmition;
			JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
			if (lock.Succeeded()) { // body_id may no longer be valid
				const JPH::Body& body = lock.GetBody();
				JPH::RVec3 position = body.GetPosition();
				transmition.position = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1.0f);
			}
			if (colourView.contains(entity)) {
				colourComponent colour = colourView.get < colourComponent>(entity);
				transmition.colour = glm::vec4(colour.colour, 1.0f);
			}
			else {
				transmition.colour = glm::vec4(1.0f); // Default colour if no colour component found

			}
			if (directionView.contains(entity)) {
				directionComponent direction = directionView.get < directionComponent>(entity);
				transmition.direction = glm::vec4(direction.direction, 0.0f);
			}
			else {
				transmition.direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f); // Default direction if no direction component found
			}
			if (light.range) {
				transmition.radius = glm::vec4(light.range, 0.0f, 0.0f, 0.0f);
			}
			else {
				transmition.radius = glm::vec4(1.0f); // Default radius if no range specified

			}
			lightTransmitions->push_back(transmition);
			});
		return lightTransmitions;
	}
	// Use .each() to smoothly unpack components by reference
	//for (auto [entity, light, jolt] : view.each()) {
	//	lightTransmition transmition;
	//	//transmition.luminosity = light.luminosity;
	//	JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
	//	if (lock.Succeeded()) { // body_id may no longer be valid
	//		const JPH::Body& body = lock.GetBody();
	//		JPH::RVec3 position = body.GetPosition();
	//		transmition.position = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1.0f);
	//	}
	//	if (colourView.contains(entity)) {
	//		colourComponent colour = colourView.get < colourComponent>(entity);
	//		transmition.colour = glm::vec4(colour.colour, 1.0f);
	//	}
	//	else {
	//		transmition.colour = glm::vec4(1.0f); // Default colour if no colour component found

	//	}
	//	if (directionView.contains(entity)) {
	//		directionComponent direction = directionView.get < directionComponent>(entity);
	//		transmition.direction = glm::vec4(direction.direction, 0.0f);
	//	}
	//	else {
	//		transmition.direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f); // Default direction if no direction component found
	//	}
	//	if (light.range) {
	//		transmition.radius = glm::vec4(light.range, 0.0f, 0.0f, 0.0f);
	//	}
	//	else {
	//		transmition.radius = glm::vec4(1.0f); // Default radius if no range specified

	//	}
	//	lightTransmitions->push_back(transmition);
	//}



}