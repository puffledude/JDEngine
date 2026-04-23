#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>
namespace JD
{
	const int MAX_FRAMES_IN_FLIGHT = 2;

	struct PerFrame {
		vk::Fence renderFence;
		vk::Semaphore presentSemaphore;
		vk::CommandBuffer commandBuffer;
	};

	struct Queues{
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
	};

	struct VkbStuff{
		//vkb::Instance instance;
		vkb::Device device;
		vkb::Swapchain swapChain;
		vk::SurfaceKHR surface = nullptr;
	};

	struct VulkanCore {
		vkb::Instance instance;
		vk::Device device;
		vk::SwapchainKHR swapChain;
		vk::SurfaceKHR surface = nullptr;
		VmaAllocator allocator = nullptr;
		vk::CommandPool commandPool;
		std::vector<vk::CommandBuffer> commandBuffers;
		std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame;
		std::vector<vk::Image> swapChainImages;
		std::vector<vk::ImageView> swapChainImageViews;
		std::vector<vk::Semaphore> renderSemaphores;

		vk::Sampler textureSampler;
		VkbStuff vkbInstances;
		Queues queues;
		~VulkanCore() {
			for (const PerFrame& var : perFrame)
			{
				// Use the neatly wrapped vulkan.hpp C++ device methods
				if (var.renderFence) device.destroy(var.renderFence);
				if (var.presentSemaphore) device.destroy(var.presentSemaphore);
			}

			if (commandPool) {
				device.destroy(commandPool);
			}

			if (allocator != nullptr) {
				vmaDestroyAllocator(allocator);
			}

			// Ensure we are passing the vkb:: struct variants back to vkb::destroy
			vkb::destroy_swapchain(vkbInstances.swapChain);
			vkb::destroy_device(vkbInstances.device);
			vkb::destroy_surface(instance, surface);
			vkb::destroy_instance(instance);
		}
	};


	//Core for rendering meshes with instanceing
	struct MeshInstanceBatch {
		uint32_t ssboBaseOffset;        // where this mesh's matrices start in the SSBO
		uint32_t instanceCount;			//How many instances of this mesh are being rendered
		std::vector<glm::mat4> modelMatrices; // model matrices for each instance of this mesh
		std::vector<MeshComponent*> pieces; // all pieces that belong to this mesh
	};

}
