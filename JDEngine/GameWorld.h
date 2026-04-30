#pragma once
#include <vector>
#include "Components.h"
#include "Controller.h"
#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace JD {
	class Camera;
	class Gameworld {
	public:
		Gameworld(JPH::PhysicsSystem* physicsSystem, Controller* controller);
		~Gameworld();
		void Update(float dt);
		entt::entity* CreateEntity() {
			return  new entt::entity(registry->create());
		}

		entt::registry* GetRegistry() {
			return registry;
		}
		std::vector<RenderableComponent>* GetRenderobjects() {
			std::vector<RenderableComponent>* renderObjects = new std::vector<RenderableComponent>();
			auto view = registry->view<RenderableComponent>();
			for (auto entity : view) {
				renderObjects->push_back(view.get<RenderableComponent>(entity));
			}
			return renderObjects;
		}
		std::vector<RenderTransmition>* getRenderTransmitions();
		std::vector<lightTransmition>* getLighTransmitions();


		std::vector<RenderableComponent>* getallRenderableComponents() {
			std::vector<RenderableComponent>* renderableComponents = new std::vector<RenderableComponent>();
			auto view = registry->view<RenderableComponent>();
			for (auto entity : view) {
				renderableComponents->push_back(view.get<RenderableComponent>(entity));
			}
			return renderableComponents;
		}
		glm::vec3 getCameraPosition();
		glm::mat4 getCameraView();
		glm::mat4 getSunView();
		glm::vec3 getSunPosition();

		lightTransmition* getSun() {
			const JPH::BodyLockInterface& lock_interface = physicsSystem->GetBodyLockInterface(); // Or GetBodyLockInterfaceNoLock
			auto view = registry->view<sunComponent, colourComponent, directionComponent, lightComponent, JoltComponent>();
			for (auto [sun, colour, direction, light, jolt] : view.each()) {
				JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
				if (lock.Succeeded()) { // body_id may no longer be valid
					lightTransmition* sunTransmition = new lightTransmition();
					const JPH::Body& body = lock.GetBody();
					JPH::RVec3 position = body.GetPosition();
					sunTransmition->position = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1);
					sunTransmition->direction = glm::vec4(direction.direction, 0.0f);
					sunTransmition->colour = glm::vec4(colour.colour, 1.0f);
					//sunTransmition->luminosity = light.luminosity;
					return sunTransmition;
				}

			}
			return nullptr;
		}
	

		JPH::PhysicsSystem* GetPhysicsSystem() {
			return physicsSystem;
		}

	protected:
		entt::registry* registry;
		JPH::PhysicsSystem* physicsSystem;
		Controller* controller;
		Camera* camera;
	};
};

		/*std::vector<RenderObject*> GetRenderObjects() {
			std::vector<RenderObject*> renderObjects;
			for (GameObject* gameObject : gameObjects) {
				RenderObject* renderObj = gameObject->GetRenderObject();
				if (renderObj != nullptr) {
					renderObjects.push_back(renderObj);
				}
			}
			return renderObjects;*
		/*void addGameObject(GameObject* gameObject) {
			gameObjects.push_back(gameObject);
		};*/

		


