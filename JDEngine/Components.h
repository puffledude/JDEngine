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

	struct TransformComponent {
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		TransformComponent() : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}
		TransformComponent* parent = nullptr;
		// Trivially destructible
	};

	struct BasicShapeComponent {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		// std::vector cleans up its own memory automatically.
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
		float metallicFactor = 0;
		float roughnessFactor = 0;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		
		Texture baseColorTexture;
		TextureView baseColorTextureView;
		VmaAllocation baseColorTextureAllocation = VK_NULL_HANDLE;

		Texture normalTexture;
		TextureView normalTextureView;
		VmaAllocation normalTextureAllocation = VK_NULL_HANDLE;

		// Appropriate manual "destructor" requiring necessary Vulkan context
		void Destroy(vk::Device device, VmaAllocator allocator) {
			if (baseColorTextureView) {
				device.destroyImageView(baseColorTextureView);
				baseColorTextureView = nullptr;
			}
			if (baseColorTexture && baseColorTextureAllocation) {
				vmaDestroyImage(allocator, baseColorTexture, baseColorTextureAllocation);
				baseColorTexture = nullptr;
				baseColorTextureAllocation = VK_NULL_HANDLE;
			}

			if (normalTextureView) {
				device.destroyImageView(normalTextureView);
				normalTextureView = nullptr;
			}
			if (normalTexture && normalTextureAllocation) {
				vmaDestroyImage(allocator, normalTexture, normalTextureAllocation);
				normalTexture = nullptr;
				normalTextureAllocation = VK_NULL_HANDLE;
			}
		}
	};

	struct MeshComponent {
		uint32_t id = 0; 
		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		
		// Note: VmaAllocations for vertexBuffer and indexBuffer are missing here 
		// but would be needed to fully clean up the buffers!
		VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;
		VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;

		Material material;

		// Appropriate manual "destructor" requiring necessary Vulkan context
		void Destroy(vk::Device device, VmaAllocator allocator) {
			material.Destroy(device, allocator);

			if (vertexBuffer && vertexBufferAllocation) {
				vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
				vertexBuffer = nullptr;
				vertexBufferAllocation = VK_NULL_HANDLE;
			}

			if (indexBuffer && indexBufferAllocation) {
				vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
				indexBuffer = nullptr;
				indexBufferAllocation = VK_NULL_HANDLE;
			}
		}
	};

	struct JoltComponent {
		JPH::BodyID bodyID;
		// Destroying physics bodies is usually handled by the PhysicsSystem
		// removing the BodyID from the JPH::PhysicsSystem interface.
	};

	struct GPUObjectData {
		glm::mat4 model;
	};

	struct CameraInfo {
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
	};

	struct RenderableComponent {
		std::vector<MeshComponent>* mesh = nullptr;
		// Pointer destructors do not delete. If this struct owns the mesh vector, 
		// consider dynamically allocating it using std::unique_ptr instead of raw pointers.
	};

	struct RenderTransmition {
		std::vector<MeshComponent>* mesh = nullptr;
		glm::mat4 modelMatrix;
	};

}

