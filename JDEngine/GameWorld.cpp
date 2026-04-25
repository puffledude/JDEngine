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
