#pragma once
#include "Gameworld.h"


namespace JD
{
	class Renderer {

		virtual void Init(Gameworld& gameworld) = 0;
		/*virtual Renderer() = 0;
		virtual ~Renderer() = 0;*/
	protected:

		virtual void Update(float dt) = 0;


		Gameworld& gameworld;
	};
}
