#include "VulkanRenderer.h"
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"
#include "RequiredFeatures.h"



namespace JD
{
	VulkanRenderer::VulkanRenderer(Gameworld& world):Renderer(world){
	}
	
	void VulkanRenderer::AssignWindow(GLFWwindow* window) {
		this->window = window;
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		initVulkan();
	}
	void VulkanRenderer::initVulkan() {
		try {
			vkb::InstanceBuilder builder;
			auto inst_ret = builder.set_app_name("JDEngine").request_validation_layers()
				.use_default_debug_messenger().build();
			if (!inst_ret) {
				throw std::runtime_error("Failed to create Vulkan instance: " + inst_ret.error().message());
			}
			vulkanCore.instance = inst_ret.value();

			vulkanCore.surface = create_surface_glfw(vulkanCore.instance.instance, window);
			createDevices();

		}
		catch (const std::exception& e) {
			std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void VulkanRenderer::createDevices() {
		vkb::PhysicalDeviceSelector physDeviceSelector(vulkanCore.instance);
		auto phys_device_ret = physDeviceSelector.set_surface(vulkanCore.surface).set_minimum_version(1, 3)
			.set_required_features_11(JD::RequiredPhysicalFeatures().vulkan11Features)
			.set_required_features_13(JD::RequiredPhysicalFeatures().vulkan13Features)
			.add_required_extension_features(JD::RequiredPhysicalFeatures().extendedDynamicStateFeatures)
			.select();
		if (!phys_device_ret) {
			std::cout << phys_device_ret.error().message() << "\n";
			if (phys_device_ret.error() == vkb::PhysicalDeviceError::no_suitable_device) {
				const auto& detailed_reasons = phys_device_ret.detailed_failure_reasons();
				if (!detailed_reasons.empty()) {
					std::cerr << "GPU Selection failure reasons:\n";
					for (const std::string& reason : detailed_reasons) {
						std::cerr << reason << "\n";
					}
				}
			}
			throw std::runtime_error("Failed to select physical device: " + phys_device_ret.error().message());
		}
		vkb::PhysicalDevice physDevice = phys_device_ret.value();
		vkb::DeviceBuilder deviceBuilder(physDevice);
		auto device_ret = deviceBuilder.build();
		if (!device_ret) {
			std::cout << device_ret.error().message() << "\n";
			return;
		}
		vulkanCore.device = device_ret.value();

	}

	void VulkanRenderer::createSwapChain() {

		vkb::SwapchainBuilder swapchainBuilder(vulkanCore.device);
		auto swap_ret = swapchainBuilder.set_old_swapchain(vulkanCore.swapChain).build();
		if (!swap_ret) {
			std::cout << swap_ret.error().message() << " " << swap_ret.vk_result() << "\n";
			return;
		}
		vkb::destroy_swapchain(vulkanCore.swapChain);
		vulkanCore.swapChain = swap_ret.value();
	}

	void VulkanRenderer::initVMA() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = vulkanCore.device.physical_device;
		allocatorInfo.device = vulkanCore.device.device;
		allocatorInfo.instance = vulkanCore.instance.instance;
		if (vmaCreateAllocator(&allocatorInfo, &vulkanCore.allocator) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VMA allocator");
		}
	}


	void VulkanRenderer::createImageViews() {
	}

	void VulkanRenderer::createGraphicsPipelines() {


	}


	void VulkanRenderer::Update(float dt) {
		drawFrame();
	}

	void VulkanRenderer::drawFrame() {
		//Get fences and semaphores for the current frame
		//Then bring in the render objects to the command buffer and submit the command buffer to the graphics queue


	}








}
