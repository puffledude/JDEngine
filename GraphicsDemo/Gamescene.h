#include "GameWorld.h"
#include "Renderer.h"


// Plan is to use this to be where I call all the abstracted commands from the engine below.
// Using the ECS, update all the game objects, then pass them through to the renderer to be rendered.


class GameScene {
public:
    GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer);
    ~GameScene();

	void Update(float dt);
private:
    JD::Gameworld* gameWorld;
    JD::Renderer* renderer;


	std::vector<JD::MeshComponent> tree;
};