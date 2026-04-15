#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>
namespace JD
{
	const int MAX_FRAMES_IN_FLIGHT = 2;

	struct PerFrame {
		vk::Fence renderFence;
		vk::Semaphore presentSemaphore;
		vk::Semaphore renderSemaphore;
		vk::CommandBuffer commandBuffer;
	};

	struct Queues{
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
	};

	struct VulkanCore {
		vkb::Instance instance;
		vkb::Device device;
		vkb::Swapchain swapChain;
		vk::SurfaceKHR surface = nullptr;
		VmaAllocator allocator = nullptr;
		vk::CommandPool commandPool;
		std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame;
		std::vector<vk::Image> swapChainImages;
		std::vector<vk::ImageView> swapChainImageViews;
		Queues queues;
		~VulkanCore() {
			for (PerFrame var : perFrame)
			{
				// Use the underlying VkDevice for the C API, and cast the C++ wrapper handles
				if (var.renderFence) vkDestroyFence(device.device, static_cast<VkFence>(var.renderFence), nullptr);
				if (var.presentSemaphore) vkDestroySemaphore(device.device, static_cast<VkSemaphore>(var.presentSemaphore), nullptr);
				if (var.renderSemaphore) vkDestroySemaphore(device.device, static_cast<VkSemaphore>(var.renderSemaphore), nullptr);
			}

			if (allocator != nullptr) {
				vmaDestroyAllocator(allocator);
			}

			vkb::destroy_swapchain(swapChain);
			vkb::destroy_device(device);
			vkb::destroy_surface(instance, surface);
			vkb::destroy_instance(instance);
		}
	};
}
