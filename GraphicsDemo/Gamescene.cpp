#include "GameScene.h"
#include "Components.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace JD;
GameScene::GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer) : gameWorld(gameWorld), renderer(renderer){
    environmentEntity = gameWorld->CreateEntity();
    scaleComponent scale{};
	scale.scale = glm::vec3(10.0f);
	renderer->loadGLTF(Environment, GLTFDIR "/Environment/CourseWorkProject.gltf");
    gameWorld->GetRegistry()->emplace<JD::RenderableComponent>(*environmentEntity, &Environment);
	gameWorld->GetRegistry()->emplace<JD::scaleComponent>(*environmentEntity, scale);
    JPH::BoxShapeSettings test  = JPH::BoxShapeSettings(JPH::Vec3(10.0f, 10.0f, 10.0f));
    JPH::ShapeSettings::ShapeResult Result = test.Create();
    JPH::ShapeRefC boxShape = Result.Get();
	JPH::BodyCreationSettings bodySettings(boxShape, JPH::RVec3(0.0f, 0.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
	JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
	gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
	gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*environmentEntity, body->GetID());
}

GameScene::~GameScene() {
	renderer->DestroyMesh(Environment);
}

void GameScene::Update(float dt) {
    //In the update loop, only update things we want such as game objects know wil be active.
    // Update game world

    /*
    Graphics loop plan idea.
    Get the ubos from the renderComponents. 
    Pass them to the renderer to be processed before any drawing.
	Rendermeshes contain both the ubo data and their index into the stoage buffer.
    Then pass the meshcompnent vector to the renderer to be processed.
    The meshcomponents will be supplied with only the index as the update has already happend.
    */


    // Render the scene
    //renderer->Render(gameWorld);
}