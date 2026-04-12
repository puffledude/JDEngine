#pragma once
#include "Renderer.h"
#include "GameWorld.h"

namespace JD
{
	class VulkanRenderer : Renderer {
	public:
		VulkanRenderer();
		void Init(Gameworld& world) override;

		~VulkanRenderer();
	protected:

		void Update(float dt) override;

	private:



	};
};
