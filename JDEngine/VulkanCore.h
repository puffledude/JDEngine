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

		// Do not perform complex destruction here; cleanup must be done explicitly and in a well-defined order.
		//~VulkanCore() noexcept = default;
	};


	//Core for rendering meshes with instanceing
	struct MeshInstanceBatch {
		uint32_t ssboBaseOffset;        // where this mesh's matrices start in the SSBO
		uint32_t instanceCount;			//How many instances of this mesh are being rendered
		std::vector<glm::mat4> modelMatrices; // model matrices for each instance of this mesh
		std::vector<MeshComponent*> pieces; // all pieces that belong to this mesh
	};

	struct Skybox {

		vk::DescriptorSetLayout skyboxDescriptorSetLayout = nullptr;
		std::vector<vk::DescriptorSet> skyboxDescriptorSets;
		vk::Pipeline skyboxPipeline = nullptr;
		vk::PipelineLayout skyboxPipelineLayout = nullptr;
		vk::Image skyboxImage = nullptr;
		VmaAllocation skyboxAllocation = nullptr;
		vk::ImageView skyboxImageView = nullptr;
		vk::Image skyboxRenderOutputImage = nullptr;
		VmaAllocation skyboxRenderOutputAllocation = nullptr;
		vk::ImageView skyboxRenderOutputView = nullptr;
	};

}
