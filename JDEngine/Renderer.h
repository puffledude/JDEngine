#pragma once
#include "Gameworld.h"
#include "tiny_gltf.h"
#include "Components.h"
#include "Vertex.h"


namespace JD
{
	class Renderer {
		public:
			Renderer(Gameworld& gameworld) : gameworld(gameworld) {};
			virtual void Update(float dt) = 0;
			virtual void loadGLTF(std::vector<MeshComponent>& meshComponents, std::string filePath) = 0;
			virtual void DestroyMesh(std::vector<MeshComponent>& mesh) = 0;
			//virtual tinygltf::Scene* makeGLTFScene(GLTFData data) = 0;

		protected:
		Gameworld& gameworld;
	};
}
