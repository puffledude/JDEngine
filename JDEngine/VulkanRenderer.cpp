#include "VulkanRenderer.h"
//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#define VMA_IMPLEMENTATION
// Now include the headers so they catch the defines above!
#include "tiny_gltf_v3.h"
#include "vma/vk_mem_alloc.h"

// Then your project headers

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
		/*if (vulkanCore.device.device != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(vulkanCore.device.device);
		}*/
		vulkanCore.device.waitIdle();

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
			
			// Initialize dispatcher with vkGetInstanceProcAddr globally
			VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanCore.instance.fp_vkGetInstanceProcAddr);
			// Initialize dispatcher with instance level functions
			VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanCore.instance.instance, vulkanCore.instance.fp_vkGetInstanceProcAddr);

			vulkanCore.surface = create_surface_glfw(vulkanCore.instance, window);
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
		vulkanCore.vkbInstances.device = device_ret.value();
		vulkanCore.device = vulkanCore.vkbInstances.device;

		// 3. Load device-level extension functions into vulkan.hpp
		VULKAN_HPP_DEFAULT_DISPATCHER.init(
			vulkanCore.instance.instance,
			vulkanCore.instance.fp_vkGetInstanceProcAddr,
			vulkanCore.vkbInstances.device.device,
			vulkanCore.vkbInstances.device.fp_vkGetDeviceProcAddr
		);
	}

	void VulkanRenderer::createSwapChain() {

		vkb::SwapchainBuilder swapchainBuilder(vulkanCore.vkbInstances.device);

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
		vkb::destroy_swapchain(vulkanCore.vkbInstances.swapChain);
		vulkanCore.vkbInstances.swapChain = swap_ret.value();
		vulkanCore.swapChain = vulkanCore.vkbInstances.swapChain;
		vulkanCore.swapChainImages = vulkanCore.device.getSwapchainImagesKHR(vulkanCore.swapChain);
	}

	void VulkanRenderer::initVMA() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = vulkanCore.vkbInstances.device.physical_device;
		allocatorInfo.device = vulkanCore.device;
		allocatorInfo.instance = vulkanCore.instance.instance;
		if (vmaCreateAllocator(&allocatorInfo, &vulkanCore.allocator) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VMA allocator");
		}
	}

	vk::ImageView VulkanRenderer::createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
		vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
			.format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
		viewInfo.subresourceRange.levelCount = mipLevels;
		return vulkanCore.device.createImageView(viewInfo);		//return vk::ImageView(static_cast<vk::Device>(vulkanCore.device.device), viewInfo);
	}


	void VulkanRenderer::createImageViews() {
		assert(vulkanCore.swapChainImageViews.empty());
		vulkanCore.swapChainImageViews.reserve(vulkanCore.swapChainImages.size());

		for (uint32_t i = 0; i < vulkanCore.swapChainImages.size(); i++) {
			vulkanCore.swapChainImageViews.push_back(createImageView(vulkanCore.swapChainImages[i], static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format), vk::ImageAspectFlagBits::eColor, 1));
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
		vk::CommandBuffer commandBuffer = vulkanCore.device.allocateCommandBuffers(allocInfo).front();
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
		vulkanCore.queues.graphicsQueue = vulkanCore.vkbInstances.device.get_queue(vkb::QueueType::graphics).value();
		vulkanCore.queues.presentQueue = vulkanCore.vkbInstances.device.get_queue(vkb::QueueType::present).value();
	}

	void VulkanRenderer::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = vulkanCore.vkbInstances.device.get_queue_index(vkb::QueueType::graphics).value()};

		vulkanCore.commandPool = vulkanCore.device.createCommandPool(poolInfo);
	}



	void VulkanRenderer::createGraphicsPipelines() {
	}


	void VulkanRenderer::Update(float dt) {
		drawFrame();
	}

	void VulkanRenderer::drawFrame() {
		//Get fences and semaphores for the current frame
		//Then bring in the render objects to the command buffer and submit the command buffer to the graphics queue


		//auto fenceResult = vkWaitForFences(vulkanCore.device, 1, inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
		//if (fenceResult != VK_SUCCESS)
		//{
		//	throw std::runtime_error("failed to wait for fence!");
		//}

		//auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

		//if (result == vk::Result::eErrorOutOfDateKHR)
		//{
		//	std::cout << "Swap chain is out of date, recreating swap chain..." << std::endl;
		//	recreateSwapChain();
		//	return;
		//}
		//if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		//{
		//	assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
		//	throw std::runtime_error("failed to acquire swap chain image!");
		//}

		//// Only reset the fence if we are submitting work
		//device.resetFences(*inFlightFences[frameIndex]);

		////updateUniformBuffer(frameIndex);
		//recordCommandBuffer(imageIndex);
		//vk::PipelineStageFlags waitDestinationStageMask = (vk::PipelineStageFlagBits::eColorAttachmentOutput);
		//const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1,
		//						  .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
		//						  .pWaitDstStageMask = &waitDestinationStageMask,
		//						  .commandBufferCount = 1,
		//						  .pCommandBuffers = &*commandBuffers[frameIndex],
		//						  .signalSemaphoreCount = 1,
		//						  .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] };

		//graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

		//const vk::PresentInfoKHR presentInfo{
		//	.waitSemaphoreCount = 1,
		//	.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
		//	.swapchainCount = 1,
		//	.pSwapchains = &*swapChain,
		//	.pImageIndices = &imageIndex,
		//	.pResults = nullptr
		//};
		////result = graphicsQueue.presentKHR(presentInfo);
		////This is dumb. The c++ bindings throw an exception if the result isn't esuccess or suboptimal.
		//result = static_cast<vk::Result>(
		//	graphicsQueue.getDispatcher()->vkQueuePresentKHR(static_cast<VkQueue>(*graphicsQueue), reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)));


		//if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR) || framebufferResized) {
		//	std::cout << "Swap chain is out of date or suboptimal, recreating swap chain..." << std::endl;
		//	framebufferResized = false;
		//	recreateSwapChain();
		//}
		//else {
		//	assert(result == vk::Result::eSuccess);
		// }
		//frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;



	}


	tinygltf::Scene* VulkanRenderer::makeGLTFScene(GLTFData data) {
		return nullptr;
	}

}