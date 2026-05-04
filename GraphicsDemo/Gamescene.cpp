#include "GameScene.h"
#include "Components.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace JD;
GameScene::GameScene(JD::Gameworld* gameWorld, JD::Renderer* renderer) : gameWorld(gameWorld), renderer(renderer){
	loadEnvironment();
    addLights();
	addTrees();
}

GameScene::~GameScene() {
	renderer->DestroyMesh(Environment);
    renderer->DestroyMesh(treeMesh);
}


void GameScene::loadEnvironment() {
    environmentEntity = gameWorld->CreateEntity();
    scaleComponent scale{};
    scale.scale = glm::vec3(10.0f);
    renderer->loadGLTF(Environment, GLTFDIR "/Environment/CourseWorkProject.gltf");
    gameWorld->GetRegistry()->emplace<JD::RenderableComponent>(*environmentEntity, &Environment);
    gameWorld->GetRegistry()->emplace<JD::scaleComponent>(*environmentEntity, scale);
    JPH::BoxShapeSettings test = JPH::BoxShapeSettings(JPH::Vec3(10.0f, 10.0f, 10.0f));
    JPH::ShapeSettings::ShapeResult Result = test.Create();
    JPH::ShapeRefC boxShape = Result.Get();
    JPH::BodyCreationSettings bodySettings(boxShape, JPH::RVec3(-5.0f, -10.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
    JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
    gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
    gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*environmentEntity, body->GetID());
}

void GameScene::addLights() {
	glm::vec3 lightPos = glm::vec3(8.86918f, -2.857519f, 4.66553f);
    glm::vec3 viewDir = glm::normalize(glm::vec3(-2.16188, -5.90516, 4.87608) - lightPos);
    sun = addLight(lightPos, glm::vec3(1.0f), viewDir, 50.0f);
    gameWorld->GetRegistry()->emplace<JD::sunComponent>(*sun);
	//entt::entity* pointLight = addLight(glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), 6.0f);
}

entt::entity* GameScene::addLight(glm::vec3 position, glm::vec3 color, glm::vec3 direction, float range) {
    entt::entity* lightEntity = gameWorld->CreateEntity();
	JPH::SphereShapeSettings ball = JPH::SphereShapeSettings(0.1f);
	JPH::ShapeSettings::ShapeResult Result = ball.Create();
	JPH::BodyCreationSettings bodySettings(Result.Get(), JPH::RVec3(position.x, position.y, position.z), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
	JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
	gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
	gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*lightEntity, body->GetID());
    gameWorld->GetRegistry()->emplace<JD::colourComponent>(*lightEntity, color);
    gameWorld->GetRegistry()->emplace<JD::directionComponent>(*lightEntity, direction);
    gameWorld->GetRegistry()->emplace<JD::lightComponent>(*lightEntity, range);
    return lightEntity;

}

void GameScene::addTrees() {
    std::vector<glm::vec3> positions = {
        glm::vec3(1.70068, -5.89332, 4.95399),
        glm::vec3(-1.77826, -5.89148, 3.73665),
        glm::vec3(-1.94088, -5.86022, 5.71867),
        glm::vec3(1.77775, -6.03859, 3.06386)
    };
	renderer->loadGLTF(treeMesh,GLTFDIR"/Tree/Tree.gltf");
    for(glm::vec3 pos : positions) {
        entt::entity* treeEntity = gameWorld->CreateEntity();
        gameWorld->GetRegistry()->emplace<JD::RenderableComponent>(*treeEntity, &treeMesh);
        scaleComponent scale{};
        scale.scale = glm::vec3(0.05f);
        gameWorld->GetRegistry()->emplace<JD::scaleComponent>(*treeEntity, scale);
        
        JPH::BoxShapeSettings test = JPH::BoxShapeSettings(JPH::Vec3(10.0f, 10.0f, 10.0f));
        JPH::ShapeSettings::ShapeResult Result = test.Create();
        JPH::ShapeRefC boxShape = Result.Get();
        JPH::BodyCreationSettings bodySettings(boxShape, JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
        JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
        gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
        gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*treeEntity, body->GetID());


        treeEntities.push_back(treeEntity);
	}

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