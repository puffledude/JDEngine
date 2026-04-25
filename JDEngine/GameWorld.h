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

		std::vector<RenderableComponent>* getallRenderableComponents() {
			std::vector<RenderableComponent>* renderableComponents = new std::vector<RenderableComponent>();
			auto view = registry->view<RenderableComponent>();
			for (auto entity : view) {
				renderableComponents->push_back(view.get<RenderableComponent>(entity));
			}
			return renderableComponents;
		}
        glm::mat4 getCameraView();
		

		/*CameraInfo* getCameraInfo() {
			CameraInfo* cameraInfo = new CameraInfo();
			cameraInfo->view = glm::mat4(1.0f);
			cameraInfo->projection = glm::mat4(1.0f);
			return cameraInfo;
		}*/

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

		


