#pragma once
#include <vector>
#include "Components.h"
#include <entt/entt.hpp>


namespace JD {
	class Gameworld {
	public:
		Gameworld();
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
		std::vector<RenderTransmition>* CreateRenderTransmition();


	protected:
		entt::registry* registry;
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

		


