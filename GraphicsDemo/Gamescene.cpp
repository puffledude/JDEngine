#include "GameScene.h"
#include "Components.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace JD;
GameScene::GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer, JPH::PhysicsSystem* physics) : gameWorld(gameWorld), renderer(renderer), physicsSystem(physics){
    environmentEntity = gameWorld->CreateEntity();
	renderer->loadGLTF(Environment, GLTFDIR "/Environment/CourseWorkProject.gltf");


}

GameScene::~GameScene() {
}

void GameScene::Update(float dt) {
    //In the update loop, only update things we want such as game objects know wil be active.
    // Update game world
    gameWorld->Update(dt);

    // Render the scene
    //renderer->Render(gameWorld);
}