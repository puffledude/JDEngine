#include "GameWorld.h"
namespace JD
{
	Gameworld::Gameworld() {
		registry = new entt::registry();
	}

	Gameworld::~Gameworld() {
		delete registry;
	}
	void Gameworld::Update(float dt) {
		// Update all game objects in the world
		// for (GameObject* gameObject : gameObjects) {
		// 	gameObject->Update(dt);
		// }
	}

}

