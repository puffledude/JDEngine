#include "GameScene.h"
#include "Components.h"

using namespace JD;
GameScene::GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer) : gameWorld(gameWorld), renderer(renderer) {

    std::vector<MeshComponent> treeMeshComponents{};
	renderer->loadGLTF(treeMeshComponents, GLTFDIR "/Environment/CourseWorkProject.gltf");



}

GameScene::~GameScene() {
}

void GameScene::Update(float dt) {
    // Update game world
    gameWorld->Update(dt);

    // Render the scene
    //renderer->Render(gameWorld);
}