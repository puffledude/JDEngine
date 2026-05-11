#include <entt/entt.hpp>
#include "Components.h"
#include <random>
namespace JD {
	class ParticleSystem {

	public:

		void Update(entt::registry& registry, float dt) {
			std::vector<entt::entity> toRemove;
			registry.view<ParticleComponent>().each([&](auto entity, ParticleComponent& particle) {
				particle.Update(dt);
				if (particle.lifetime <= 0.0f) {
					toRemove.push_back(entity);
				}
				});
			for (entt::entity entity : toRemove) {
				registry.destroy(entity);
			}
		}
	};

	class ParticleEmitterSystem {
		public:
			ParticleEmitterSystem(uint32_t maxParticles) : maxParticles(maxParticles) {
				

			}
			void Update(entt::registry& registry, float dt) {
				auto view = registry.view<ParticleEmitterComponent>();
				for (auto entity : view) {
					auto& emitter = view.get<ParticleEmitterComponent>(entity);
					for (uint32_t i = 0; i < emitter.emitionsPerFrame; ++i) {
						if (registry.view<ParticleComponent>().size() >= maxParticles) {
							break; // Limit the number of particles
						}
						entt::entity* particleEntity = new entt::entity(registry.create());
						std::uniform_real_distribution<float> velDist(0.0f, 4.0f);
						std::uniform_real_distribution<float> lifeDist(0.1f, 0.5f);

						/*entt::entity particleEntity = registry.create();
						float angleX = (rand() / (float)RAND_MAX - 0.5f) * emitter.angleRange.x;
						float angleY = (rand() / (float)RAND_MAX - 0.5f) * emitter.angleRange.y;
						float angleZ = (rand() / (float)RAND_MAX - 0.5f) * emitter.angleRange.z;
						glm::vec3 direction = glm::rotateX(emitter.up, glm::radians(angleX));
						direction = glm::rotateY(direction, glm::radians(angleY));
						direction = glm::rotateZ(direction, glm::radians(angleZ));
						float speed = emitter.speedRange.first + static_cast<float>(rand()) / RAND_MAX * (emitter.speedRange.second - emitter.speedRange.first);
						registry.emplace<ParticleComponent>(particleEntity, ParticleComponent{ .lifetime = 1.0f, .startLifetime = 1.0f, .id = static_cast<uint32_t>(particleEntity), .startColour = glm::vec3(1.0f), .endColour = glm::vec3(0.1f)
							});*/
					}

				}
			}
		protected:
		uint32_t maxParticles = 100;
		uint32_t nextParticleId = 0;
		std::mt19937 mt = std::mt19937((std::random_device())());
		
	};

};