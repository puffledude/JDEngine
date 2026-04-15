#pragma once
#include "Gameworld.h"
#include "RenderObject.h"
#include "tiny_gltf.h"

namespace JD
{
	class Renderer {
		public:
			Renderer(Gameworld& gameworld) : gameworld(gameworld) {};
			virtual void Update(float dt) = 0;
			virtual tinygltf::Scene* makeGLTFScene(GLTFData data) = 0;

		protected:
		Gameworld& gameworld;
	};
}
