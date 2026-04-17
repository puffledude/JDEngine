#pragma once
#include <vector>
#include "VulkanRenderer.h"
#include "tiny_gltf_v3.h"


namespace JD {
	struct GLTFData {
		std::vector<std::vector<Vertex>> vertices;  // Vector of verticies for each mesh
		std::vector<std::vector<uint32_t>> indices; // Vector of indices for each mesh
		std::vector<tinygltf::Material> materials; // Vector of materials for each mesh
	};
	//posibly need a material component/ textures component


	struct TransformComponent {
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		TransformComponent() : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}
		TransformComponent* parent = nullptr;
	};

	struct BasicShapeComponent
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	enum class RenderableType {
		GLTF,
		BasicShape
	};

	using VertexBuffer = vk::Buffer;
	using IndexBuffer = vk::Buffer;
	//Use the using keyword here to store the vertex and index buffer data with the defined vertex and index buffer types.
	//One mesh component per submesh of the gltf model. Store the submeshes materials, textures and other relevant data in the mesh component as well.
	struct MeshComponent {
		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		//std::vector<tinygltf::Material> materials; // Vector of materials for each submesh
		std::vector<tinygltf::Texture> textures; // Vector of textures for each submesh. Might change this to be image samplers instead or something.
	};


	struct RenderableComponent {
		MeshComponent mesh;
		TransformComponent transform;
		//RenderableType type;
	};
}

