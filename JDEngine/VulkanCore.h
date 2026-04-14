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
		std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame;
		~VulkanCore() {
			for (PerFrame var : perFrame)
			{
				vkDestroyFence(device, var.renderFence, nullptr);
				vkDestroySemaphore(device, var.presentSemaphore, nullptr);
				vkDestroySemaphore(device, var.renderSemaphore, nullptr);
			}
			delete (perFrame.data());

			vmaDestroyAllocator(allocator);
			vkb::destroy_swapchain(swapChain);
			vkb::destroy_device(device);
			vkb::destroy_surface(instance, surface);
			vkb::destroy_instance(instance);
			
			//instance.destroy();
		}
	};

	
}
