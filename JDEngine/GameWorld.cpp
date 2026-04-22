#include "GameWorld.h"
namespace JD
{
	Gameworld::Gameworld(JPH::PhysicsSystem* physicsSystem) : physicsSystem(physicsSystem) {
		registry = new entt::registry();
	}

	Gameworld::~Gameworld() {
		delete registry;
	}

	std::vector<RenderTransmition>* Gameworld::CreateRenderTransmition()
	{
		std::vector<RenderTransmition>* renderTransmition = new std::vector<RenderTransmition>();
		auto view = registry->view<RenderableComponent, JoltComponent>();

		// Use .each() to smoothly unpack components by reference
		for (auto [entity, renderable, jolt] : view.each()) {

			for (auto& mesh : *renderable.mesh) {
				RenderTransmition transmition;
				transmition.mesh = renderable.mesh;
				
			}
		}

		return renderTransmition;
	}
}
