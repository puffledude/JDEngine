#pragma once
#include <vector>
#include "GameObject.h"

namespace JD {
	class Gameworld {
	public:
		Gameworld() = default;
		~Gameworld();
		void Update(float dt);

		std::vector<RenderObject*> GetRenderObjects() {
			std::vector<RenderObject*> renderObjects;
			for (GameObject* gameObject : gameObjects) {
				RenderObject* renderObj = gameObject->GetRenderObject();
				if (renderObj != nullptr) {
					renderObjects.push_back(renderObj);
				}
			}
			return renderObjects;
		}
		void addGameObject(GameObject* gameObject) {
			gameObjects.push_back(gameObject);
		};

		protected:
			std::vector<GameObject*> gameObjects;

	};

}