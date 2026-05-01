#include "GameWorld.h"
#include "Renderer.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

// Plan is to use this to be where I call all the abstracted commands from the engine below.
// Using the ECS, update all the game objects, then pass them through to the renderer to be rendered.


class GameScene {
public:
    GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer);
    ~GameScene();

	void Update(float dt);
private:

	void loadEnvironment();
	void addLights();
	entt::entity* addLight(glm::vec3 position, glm::vec3 color, glm::vec3 direction, float range);

    JD::Gameworld* gameWorld;
    JD::Renderer* renderer;

	std::vector<JD::MeshComponent> Environment;
	entt::entity* environmentEntity;
	entt::entity* sun;

};