#include "GameWorld.h"
namespace JD
{
	Gameworld::Gameworld() {
		registry = new entt::registry();
	}

	Gameworld::~Gameworld() {
		delete registry;
	}

	std::vector<RenderTransmition>* Gameworld::CreateRenderTransmition()
	{
		std::vector<RenderTransmition>* renderTransmition = new std::vector<RenderTransmition>();
		auto view = registry->view<RenderableComponent, JoltComponent>();
		for (auto entity : view) {
			auto [renderable, jolt] = view.get<RenderableComponent, JoltComponent>(entity);
			for (auto& mesh : *renderable.mesh) {

				/*RenderTransmition transmition;
				transmition.mesh = renderable.mesh[0];
				transmition.uboIndex = jolt.bodyID.GetIndex();
				renderTrans*/
			}
			return renderTransmition;
		}

	}
}
