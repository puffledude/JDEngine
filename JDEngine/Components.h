#pragma once
#include <vector>
#include "Vertex.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <tiny_gltf_v3.h>
#include "vma/vk_mem_alloc.h"



namespace JD {
	//struct GLTFData {
	//	std::vector<std::vector<Vertex>> vertices;  // Vector of verticies for each mesh
	//	std::vector<std::vector<uint32_t>> indices; // Vector of indices for each mesh
	//	std::vector<Material> materials; // Vector of materials for each mesh
	//};
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
	using UniformBuffer = vk::Buffer;
	using Texture = vk::Image;
	using TextureView = vk::ImageView;

	struct Material {
		float metallicFactor =0;
		float roughnessFactor =0;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		Texture baseColorTexture;
		TextureView baseColorTextureView;
		VmaAllocation baseColorTextureAllocation;
		Texture normalTexture;
		TextureView normalTextureView;
		VmaAllocation normalTextureAllocation;
	};

	//Use the using keyword here to store the vertex and index buffer data with the defined vertex and index buffer types.
	//One mesh component per submesh of the gltf model. Store the submeshes materials, textures and other relevant data in the mesh component as well.
	struct MeshComponent {
		uint32_t id = 0;  //Used for sorting Mesh components pased to the renderer.
		//Render all objects of the same id at the same time.
		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		Material material;
		//Texture texture;


		//std::vector<tinygltf::Material> materials; // Vector of materials for each submesh
		//std::vector<tinygltf::Texture> textures; // Vector of textures for each submesh. Might change this to be image samplers instead or something.
	};

	struct JoltComponent {
		JPH::BodyID bodyID;
	};

	struct GPUObjectData {
		glm::mat4 model;
	};

	struct CameraInfo{
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
	};

	struct RenderableComponent {
		std::vector<MeshComponent>* mesh;
	};

	struct RenderTransmition {
		MeshComponent mesh;
		glm::mat4 modelMatrix;
	};

}

