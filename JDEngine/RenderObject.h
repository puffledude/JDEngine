#pragma once
#include <iostream>
#include "Vertex.h"
//#include "stb_image.h"
//#include <tiny_gltf.h>


namespace JD {
	
	enum class BasicShape {
		Quad,
		Triangle
	};

	struct GLTFData {
		std::vector<std::vector<Vertex>> vertices;  // Vector of verticies for each mesh
		std::vector<std::vector<uint32_t>> indices; // Vector of indices for each mesh
		//std::vector<tinygltf::Material> materials; // Vector of materials for each mesh
	};

	class RenderObject
	{
	public:
		//Take either an enum of a basic shape or filepath to a model
		RenderObject(std::string path);
		RenderObject(BasicShape shape);
		virtual void Update(float dt);

	};
};
