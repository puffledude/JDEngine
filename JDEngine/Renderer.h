#pragma once
#include "Gameworld.h"
#include "tiny_gltf.h"
#include "Components.h"


namespace JD
{
	class Renderer {
		public:
			Renderer(Gameworld& gameworld) : gameworld(gameworld) {};
			virtual void Update(float dt) = 0;
			virtual MeshComponent createMeshComponent() =0;
			//virtual tinygltf::Scene* makeGLTFScene(GLTFData data) = 0;

		protected:
		Gameworld& gameworld;
	};
}
