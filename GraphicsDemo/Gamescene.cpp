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
	addTent(glm::vec3(0.111249, -4.6, 6.00218), glm::vec3(0.007));
    addBoxes();
}

GameScene::~GameScene() {
	renderer->DestroyMesh(Environment);
    renderer->DestroyMesh(treeMesh);
	renderer->DestroyMesh(tentMesh);
	renderer->DestroyMesh(boxMesh);
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
	glm::vec3 lightPos = glm::vec3(-0.200569, -1.72289, -7.14662);
    glm::vec3 centre = glm::vec3(-0.301827, -7.13367, 5.66023);
	float radus = glm::length(lightPos - centre);
    glm::vec3 viewDir = glm::normalize(glm::vec3(-2.16188, -5.90516, 4.87608) - lightPos);
    sun = addLight(lightPos, glm::vec3(1.0f), viewDir, radus+10);
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

void GameScene::addTent(glm::vec3 position, glm::vec3 scale) {
    if (tentMesh.empty()) {
        renderer->loadGLTF(tentMesh, GLTFDIR "/Tent/Tent.gltf");
	}
	entt::entity* tentEntity = gameWorld->CreateEntity();
    gameWorld->GetRegistry()->emplace<JD::RenderableComponent>(*tentEntity, &tentMesh);
    scaleComponent scaleComp{};
    scaleComp.scale = scale;
    gameWorld->GetRegistry()->emplace<JD::scaleComponent>(*tentEntity, scaleComp);
    
    JPH::BoxShapeSettings test = JPH::BoxShapeSettings(JPH::Vec3(1.0f, 1.0f, 1.0f));
    JPH::ShapeSettings::ShapeResult Result = test.Create();
    JPH::ShapeRefC boxShape = Result.Get();
    JPH::BodyCreationSettings bodySettings(boxShape, JPH::RVec3(position.x, position.y, position.z), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
    JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
    gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
	gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*tentEntity, body->GetID());
}


void GameScene::addBox(glm::vec3 position, glm::vec3 scale, glm::vec3 velocity, float travelDistance) 
{
    if (boxMesh.empty()) {
        renderer->loadGLTF(boxMesh, GLTFDIR "/MovingBlock/Block.gltf");
    }
    entt::entity* boxEntity = gameWorld->CreateEntity();
    gameWorld->GetRegistry()->emplace<JD::RenderableComponent>(*boxEntity, &boxMesh);
    scaleComponent scaleComp{};
    scaleComp.scale = scale;
    gameWorld->GetRegistry()->emplace<JD::scaleComponent>(*boxEntity, scaleComp);

    JPH::BoxShapeSettings test = JPH::BoxShapeSettings(JPH::Vec3(1.0f, 1.0f, 1.0f));
    JPH::ShapeSettings::ShapeResult Result = test.Create();
    JPH::ShapeRefC boxShape = Result.Get();
    JPH::BodyCreationSettings bodySettings(boxShape, JPH::RVec3(position.x, position.y, position.z), JPH::Quat::sIdentity(), JPH::EMotionType::Kinematic, 0);
    JPH::Body* body = gameWorld->GetPhysicsSystem()->GetBodyInterface().CreateBody(bodySettings);
    gameWorld->GetPhysicsSystem()->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
    gameWorld->GetRegistry()->emplace<JD::JoltComponent>(*boxEntity, body->GetID());
	body->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
	movingboxComponent movingComp{};
	movingComp.startingPosition = position;
	movingComp.travelDistance = travelDistance;
	movingComp.velocity = velocity;
	gameWorld->GetRegistry()->emplace<JD::movingboxComponent>(*boxEntity, movingComp);
}

void GameScene::addBoxes() {
    /*std::vector<glm::vec3> positions = {
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
	};
    for (glm::vec3 pos : positions) {
        addBox(position + pos, scale, 0.0f);
	}*/
    // Camera Position: (-2.65375, -2.35872, 12.5342)
    //Camera Position : (-0.51256, -2.35872, 12.6626)
    //(-3.53448, -1.92767, 13.4512)
    //(-3.40779, -1.92767, 14.2655)
    addBox(glm::vec3(-2.65375, -2.35872, 12.5342), glm::vec3(0.5), glm::vec3(5, 0, 0), 3.0f);
    addBox(glm::vec3(-0.51256, -2.35872, 12.6626), glm::vec3(0.5), glm::vec3(0, 5, 0), 3.0f);
    addBox(glm::vec3(-3.40779, -1.92767, 14.2655), glm::vec3(0.1), glm::vec3(10, 0, 0), 2);
    addBox(glm::vec3(-3.40779, -1.92767, 14.2655), glm::vec3(0.1), glm::vec3(0, -10, 0), 2);
    addBox(glm::vec3(1.42639, 2.66094, 0.174786), glm::vec3(0.1), glm::vec3(0,  0, -10), 10);
    addBox(glm::vec3(1.42639, 2.66094, 0.274786), glm::vec3(0.1), glm::vec3(0, 0, -7), 10);
    addBox(glm::vec3(1.42639, 2.66094, 0.374786), glm::vec3(0.1), glm::vec3(0, -0, -4), 10);
    addBox(glm::vec3(1.42639, 2.66094, 0.474786), glm::vec3(0.1), glm::vec3(0, 0, -1), 10);




    
}


void GameScene::Update(float dt) {
    //In the update loop, only update things we want such as game objects know wil be active.
    // Update game world
	UpdateBoxes();
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

void GameScene::UpdateBoxes() {
    auto view = gameWorld->GetRegistry()->view<JD::movingboxComponent, JD::JoltComponent>();
    for (auto [entity, movingComp, jolt] : view.each()) {
        const JPH::BodyLockInterface& lock_interface = gameWorld->GetPhysicsSystem()->GetBodyLockInterface();
        JPH::BodyLockRead lock(lock_interface, jolt.bodyID);
        if (lock.Succeeded()) { // body_id may no longer be valid
            const JPH::Body& body = lock.GetBody();
            movingComp.Update(body);
        }
    }
}