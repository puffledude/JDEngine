#include "VulkanRenderer.h"
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"
#include "RequiredFeatures.h"



namespace JD
{
	VulkanRenderer::VulkanRenderer(Gameworld& world) :Renderer(world) {
	}

	VulkanRenderer::~VulkanRenderer() {
		cleanupVulkan();
	}

	void VulkanRenderer::cleanupVulkan() {
		// 1. Wait for everything on the GPU to finish before destroying anything
		if (vulkanCore.device.device != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(vulkanCore.device.device);
		}

		// 2. Clear out the swap chain and other abstractions
		cleanupSwapChain();
	};

	void VulkanRenderer::cleanupSwapChain() {
		vulkanCore.swapChainImages.clear();
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
			createSwapChain();
			initVMA();


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
		VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		auto swap_ret = swapchainBuilder.set_old_swapchain(vulkanCore.swapChain). set_desired_format(desiredFormat)
			.add_fallback_format({ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			 .use_default_present_mode_selection()
			 .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.build();
		if (!swap_ret) {
			std::cout << swap_ret.error().message() << " " << swap_ret.vk_result() << "\n";
			return;
		}
		vkb::destroy_swapchain(vulkanCore.swapChain);
		vulkanCore.swapChain = swap_ret.value();
		
		//std::cout << "Swapchain image format: " << vk::to_string(static_cast<vk::Format>(vulkanCore.swapChain.image_format)) << "\n";
		auto images = vulkanCore.swapChain.get_images().value();
		vulkanCore.swapChainImages = std::vector<vk::Image>(images.begin(), images.end());
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

	vk::ImageView VulkanRenderer::createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
		vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
			.format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
		viewInfo.subresourceRange.levelCount = mipLevels;
		return static_cast<vk::Device>(vulkanCore.device.device).createImageView(viewInfo);		//return vk::ImageView(static_cast<vk::Device>(vulkanCore.device.device), viewInfo);
	}


	void VulkanRenderer::createImageViews() {
		assert(vulkanCore.swapChainImageViews.empty());
		vulkanCore.swapChainImageViews.reserve(vulkanCore.swapChainImages.size());

		for (uint32_t i = 0; i < vulkanCore.swapChainImages.size(); i++) {
			vulkanCore.swapChainImageViews.push_back(createImageView(vulkanCore.swapChainImages[i], static_cast<vk::Format>(vulkanCore.swapChain.image_format), vk::ImageAspectFlagBits::eColor, 1));
		}

	}

	void VulkanRenderer::transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
		auto commandBuffer = beginSingleTimeCommands();
		vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout, .newLayout = newLayout, .image = image, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		barrier.subresourceRange.levelCount = mipLevels;

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
		endSingleTimeCommands(commandBuffer);
	}
	vk::CommandBuffer VulkanRenderer::beginSingleTimeCommands() {
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = vulkanCore.commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::CommandBuffer commandBuffer = static_cast<vk::Device>(vulkanCore.device.device).allocateCommandBuffers(allocInfo).front()	;
		//vk::CommandBuffer commandBuffer = std::move(vulkanCore.device.allocateCommandBuffers(allocInfo).front());

		vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		commandBuffer.begin(beginInfo);

		return commandBuffer;
	}
	void VulkanRenderer::endSingleTimeCommands(vk::CommandBuffer& commandBuffer) {
		commandBuffer.end();
		vulkanCore.queues.graphicsQueue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &commandBuffer }, nullptr);
		vulkanCore.queues.graphicsQueue.waitIdle();
	}




	void VulkanRenderer::createQueues() {
		vulkanCore.queues.graphicsQueue = vulkanCore.device.get_queue(vkb::QueueType::graphics).value();
		vulkanCore.queues.presentQueue = vulkanCore.device.get_queue(vkb::QueueType::present).value();
	}

	void VulkanRenderer::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = vulkanCore.device.get_queue_index(vkb::QueueType::graphics).value()};
		vulkanCore.commandPool = static_cast<vk::Device>(vulkanCore.device.device).createCommandPool(poolInfo);
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