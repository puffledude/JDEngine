#include "GameScene.h"

GameScene::GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer) : gameWorld(gameWorld), renderer(renderer) {
}

GameScene::~GameScene() {
}

void GameScene::Update(float dt) {
    // Update game world
    gameWorld->Update(dt);

    // Render the scene
    //renderer->Render(gameWorld);
}