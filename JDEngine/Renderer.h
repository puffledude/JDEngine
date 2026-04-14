#pragma once
#include "Gameworld.h"

namespace JD
{
	class Renderer {
		public:
			Renderer(Gameworld& gameworld) : gameworld(gameworld) {};
			//virtual void Init(Gameworld& gameworld) = 0;
			virtual void Update(float dt) = 0;

		/*virtual Renderer() = 0;
		virtual ~Renderer() = 0;*/
	protected:



		Gameworld& gameworld;
	};
}
