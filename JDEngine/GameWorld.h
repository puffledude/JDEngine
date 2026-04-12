#pragma once
#include <vector>
#include "GameObject.h"

namespace JD {
	class Gameworld {
	public:
		Gameworld() = default;
		~Gameworld();
		void Update(float dt);

		void addGameObject(GameObject* gameObject) {
			gameObjects.push_back(gameObject);
		};

		protected:
			std::vector<GameObject*> gameObjects;

	};

}