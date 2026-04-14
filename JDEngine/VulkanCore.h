#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>

struct VulkanCore {
	vkb::Instance instance;
	vk::PhysicalDevice physicalDevice =nullptr;
	vk::Device device =nullptr;
	vk::SwapchainKHR swapChain =nullptr;
	vk::SurfaceKHR surface = nullptr;
	VmaAllocator allocator =nullptr;
	~VulkanCore() {
		vmaDestroyAllocator(allocator);
		device.destroy();
		//instance.destroy();
	}
};