#include <iostream>
#include <chrono>
#include "VulkanRenderer.h"
#include "GameScene.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include "BroadPhaseLayers.h"
#include "WorldContactListener.h"

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
using namespace JPH;

//plan is make window here then pass surface to the renderer.
//then loop while glfw true.


class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool	ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
//namespace BroadPhaseLayers
//{
//	static constexpr BroadPhaseLayer NON_MOVING(0);
//	static constexpr BroadPhaseLayer MOVING(1);
//	static constexpr uint NUM_LAYERS(2);
//};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool				ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		// std::cout << "A body got activated" << std::endl;
	}

	virtual void		OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		// std::cout << "A body went to sleep" << std::endl;
	}
};






GLFWwindow* initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	return window;
	//glfwSetWindowUserPointer(window, this);
	//glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}


int main() {
	std::cout << "Hello, Graphics!" << std::endl;

	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	const JPH::uint cMaxPhysicsJobs = 2048;
	const JPH::uint cMaxPhysicsBarriers = 8;
	JPH::TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
	JPH::JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
	//JobSystemThreadPool* job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
	JPH::PhysicsSystem* physics = new JPH::PhysicsSystem();
	BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;

	physics->Init(1000, 16, 10000, 10000, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
	JD::Gameworld* world = new JD::Gameworld(physics);

	physics->SetContactListener(new JD::WorldContactListener(*world, *physics));
	physics->SetBodyActivationListener(new MyBodyActivationListener());
	physics->OptimizeBroadPhase();


	JD::VulkanRenderer* renderer = new JD::VulkanRenderer(*world);
	GLFWwindow* window = initWindow();
	renderer->AssignWindow(window);
	auto lastTime = std::chrono::high_resolution_clock::now();
	GameScene* scene = new GameScene(world, renderer);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;
		scene->Update(dt);
		world->Update(dt);
		physics->Update(dt, 2, &temp_allocator, &job_system);
		renderer->Update(dt);
	}
	delete scene;
	delete renderer;
	delete world;
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}