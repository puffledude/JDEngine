#pragma once
#include <vector>
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

		


