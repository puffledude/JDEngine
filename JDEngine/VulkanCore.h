#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>

struct VulkanCore {
	vkb::Instance instance;
	vkb::Device device;
	vkb::Swapchain swapChain;
	vk::SurfaceKHR surface = nullptr;
	VmaAllocator allocator =nullptr;
	~VulkanCore() {
		vmaDestroyAllocator(allocator);
		vkb::destroy_swapchain(swapChain);
		vkb::destroy_device(device);
		vkb::destroy_surface(instance, surface);
		vkb::destroy_instance(instance);

		//instance.destroy();
	}
};