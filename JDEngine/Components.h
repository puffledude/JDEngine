#pragma once
#include <vector>
#include "VulkanRenderer.h"

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
	struct MeshComponent {
		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
	};


	struct RenderableComponent {
		MeshComponent mesh;
		TransformComponent transform;
		RenderableType type;
	};
}

