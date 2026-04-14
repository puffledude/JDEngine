#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>

struct VulkanCore {
	vkb::Instance instance;
	//vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	vkb::Swapchain swapChain;
	vk::SurfaceKHR surface = nullptr;
	VmaAllocator allocator =nullptr;
	~VulkanCore() {
		vmaDestroyAllocator(allocator);
		delete surface;
		delete swapChain;
		delete device;
		delete instance;
		//instance.destroy();
	}
};